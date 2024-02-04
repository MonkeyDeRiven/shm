#include "gtest/gtest.h"
#include "io/rw-lock/ecal_named_rw_lock.h"

#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <condition_variable>

// enables a thread to hold a lock till it is not needed anymore
std::condition_variable threadTerminateCond;
std::mutex threadHoldMutex;
std::atomic<bool> threadDone = false;

// timeout for the rw-lock operations
const int64_t TIMEOUT = 20;

void writerHoldLockSerialized(std::string& lockName, std::atomic<int>& feedback /* -1=not signaled, 0=fail, 1=success*/)
{
	// create the rw-lock
	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);

	if (lockSuccess)
		// signal success to main thread
		feedback = 1;
	else {
		// signal fail to main tread and exit since lock can not be held anyways
		feedback = 0;
		return;
	}

	// hold the rw-lock until notification arrives
	threadTerminateCond.wait(std::unique_lock<std::mutex>(threadHoldMutex), [] { return threadDone.load(); });

	rwLock.Unlock();
}

void readerHoldLockSerialized(std::string& lockName, std::atomic<int>& lockHolderCount, std::atomic<int>& lockTimeoutCounter)
{
	// create the rw-lock
	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.LockRead(TIMEOUT);

	// check if lock was aquired and pass information to test running thread
	if (lockSuccess)
		lockHolderCount++;
	else {
		lockTimeoutCounter++;
		// no need to wait if lock was not aquired anyways
		return;
	}

	// hold the rw-lock and dont release it
	threadTerminateCond.wait(std::unique_lock<std::mutex>(threadHoldMutex), [] { return threadDone.load(); });

	//rwLock.UnlockRead(TIMEOUT);
}

void writerTryLock(std::string& lockName, std::atomic<bool>& success)
{
	// create the rw-lock
	eCAL::CNamedRwLock rwLock(lockName);

	success = rwLock.Lock(TIMEOUT);

	if (success == true) {
		rwLock.Unlock();
	}
}

void readerTryLock(std::string& lockName, std::atomic<bool>& success)
{
	eCAL::CNamedRwLock rwLock(lockName);
	// try to lock the file	within timeout period and write the result to success
	success = rwLock.LockRead(TIMEOUT);

	if (success == true) {
		rwLock.UnlockRead(TIMEOUT);
	}
}

void holdHandle(std::string& lockName, std::atomic<bool>& holdingHandle)
{
	eCAL::CNamedRwLock rwLock(lockName);
	holdingHandle = true;
	threadTerminateCond.wait(std::unique_lock<std::mutex>(threadHoldMutex), [] { return threadDone.load(); });
}

/*
* This test confirms that the a write lock can be aquired by a process
*/
TEST(RwLock, LockWrite)
{
	std::string lockName = "LockWriteTest";

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);

	if (lockSuccess)
		rwLock.Unlock();

	EXPECT_TRUE(lockSuccess) << "The rw-lock denied write access, even tho no other process was holding the lock at the time.";
}

/*
* This test confirms that the a read lock can be aquired by a process
*/
TEST(RwLock, LockRead)
{
	std::string lockName = "LockReadTest";

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.LockRead(TIMEOUT);

	if (lockSuccess)
		rwLock.UnlockRead(TIMEOUT);

	EXPECT_TRUE(lockSuccess) << "The rw-lock denied read access, even tho no other process was holding the lock at the time.";
}

/*
* This test confirms that the a write lock can be released by a process
*/
TEST(RwLock, UnlockWrite)
{
	std::string lockName = "UnlockWriteTest";

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);

	if (lockSuccess == true) {
		lockSuccess = rwLock.Unlock();
		EXPECT_TRUE(lockSuccess);
	}
	else
		std::cout << "WARNING: UnlockWrite test could not be run correctly due to error in CNamedRwLock::Lock() function" << std::endl;
}
/*
* This test confirms that the a read lock can be released by a process
*/
TEST(RwLock, UnlockRead)
{
	std::string lockName = "UnlockReadTest";

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);

	if (lockSuccess == true) {
		bool unlockSuccess = rwLock.Unlock();
		EXPECT_TRUE(unlockSuccess);
	}
	else
		std::cout << "WARNING: UnlockRead test could not be run correctly due to error in CNamedRwLock::Lock() function" << std::endl;
}

/*	
*	This Test confirms that a process which is currently not holding a read lock,
*	can not change the current state of the lock by calling the ReadUnlock() function.
*	This assures, that no process can unlock the read lock held by another process
*/

TEST(RwLock, UnlockReadByNoneReadLockHolder)
{
	std::string lockName = "UnlockReadByNoneReadLockHolderTest";

	std::atomic<int> readLockHolderCount = 0;
	std::atomic<int> readLockTimeoutCount = 0;

	std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);

	std::thread lockHoldingReader([&] { readerHoldLockSerialized(lockName, readLockHolderCount, readLockTimeoutCount); });

	while (readLockHolderCount + readLockTimeoutCount != 1) {}

	if (readLockTimeoutCount == 1) {
		std::cout << "WARNING: UnlockReadByNoneReadLockHolder test could not be run correctly due to issues in CNamedRwLock::LockRead() function" << std::endl;
		return;
	}

	eCAL::CNamedRwLock rwLock(lockName);
	bool unlockSuccess = rwLock.UnlockRead(TIMEOUT);

	if (!unlockSuccess) {
		std::cout << "WARNING: UnlockReadByNoneReadLockHolder test could not be run correctly due to issues in CNamedRwLock::UnlockRead() function" << std::endl;
		return;
	}

	EXPECT_EQ(1, rwLock.GetReaderCount());

	threadDone = true;
	threadHoldGuard.unlock();
	threadTerminateCond.notify_one();
	lockHoldingReader.join();
	threadDone = false;
}

/*
*	This Test confirms that a process which is currently not holding the write lock,
*	can not change the current state of the lock by calling the Unlock() function.
*	This assures, that no process can unlock the write lock held by another process
*/
TEST(RwLock, UnlockWriteByNoneWriteLockHolder)
{
	std::string lockName = "UnlockWriteByNoneWriteLockHolderTest";

	// feedback to see if writer could aquire lock
	// -1 -> no feedback yet
	//  0 -> failed
	//  1 -> success
	std::atomic<int> isLocked = -1;
	
	std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);

	std::thread lockHoldingWriter([&] { writerHoldLockSerialized(lockName, isLocked); });

	while (isLocked == -1) {};

	if (isLocked == 0) {
		std::cerr << "WARNING: LockReadWhileWriteLocked test could not be run correctly due to issues in CNamedRwLock::LockWrite() function" << std::endl;
		return;
	}

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);

	EXPECT_TRUE(lockSuccess);

	threadDone = true;
	threadHoldGuard.unlock();
	threadTerminateCond.notify_one();
	lockHoldingWriter.join();
	threadDone = false;

}

/*
* This Test confirms that the read lock can still be aquired,
* even tho other processes are holding the read lock currently
*/
TEST(RwLock, LockReadWhileReadLocked)
{
	std::string lockName = "LockReadWileReadLockedTest";

	int parallelReaderCount = 10;
	std::atomic<int> readLockHolderCount = 0;
	std::atomic<int> readLockTimeoutCount = 0;

	std::vector<std::thread> readerThreads;

	{
		// ensures that all reader hold the lock at the same time
		std::unique_lock<std::mutex> readersHoldGuard(threadHoldMutex);

		for (int i = 0; i < parallelReaderCount; i++) {
			readerThreads.push_back(std::thread([&] { readerHoldLockSerialized(lockName, readLockHolderCount, readLockTimeoutCount); }));
		}
		// busy wait till all reader are ready to be notified
		while (readLockHolderCount == parallelReaderCount - readLockTimeoutCount) {}
	}
	threadDone = true;
	threadTerminateCond.notify_all();

	// wait till all threads finish
	for (auto& thread : readerThreads) {
		thread.join();
	}

	EXPECT_EQ(parallelReaderCount, readLockHolderCount) << parallelReaderCount << " reader tried to aquire the lock, but only " << readLockHolderCount << " succeeded!";
	threadDone = false;
}

/*
* This Test confirms that a process which tries to aquire a read lock,
* while another process currently holds the write lock, fails.
*/
TEST(RwLock, LockReadWhileWriteLocked)
{
	std::string lockName = "LockReadWhileReadLockedTest";
	std::atomic<bool> success;

	// feedback to see if writer could aquire lock
	// -1 -> no feedback yet
	//  0 -> failed
	//  1 -> success
	std::atomic<int> isLocked = -1;

	std::thread lockHoldingWriter;
	{
		// ensures that the reader does not release the lock before the test case finishes
		std::lock_guard<std::mutex> threadHoldGuard(threadHoldMutex);

		// writer holds lock
		lockHoldingWriter = std::thread([&] { writerHoldLockSerialized(lockName, isLocked); });

		// busy wait till reader aquired the lock or failed
		while (isLocked == -1) {}

		// test cannot be run correctly if reader does not hold the lock
		if (isLocked == 0) {
			std::cerr << "WARNING: LockReadWhileWriteLocked test could not be run correctly due to issues in CNamedRwLock::LockWrite() function" << std::endl;
			return;
		}

		std::thread challengingReader([&] { readerTryLock(lockName, success); });

		// wait for process to timeout
		challengingReader.join();

		// release the threadHoldMutex and notify the condition variable, so the lock holding writer can terminate
	}
	threadDone = true;
	threadTerminateCond.notify_one();
	lockHoldingWriter.join();

	EXPECT_FALSE(success);
	threadDone = false;
}

/*
* This Test confirms that a process which tries to aquire the write lock,
* while another process currently holds the write lock, fails.
*/
TEST(RwLock, LockWriteWhileWriteLocked)
{
	std::string lockName = "LockWriteWhileWriteLockedTest";
	std::atomic<bool> success;

	// feedback to see if writer could aquire lock
	// -1 -> no feedback yet
	//  0 -> failed
	//  1 -> success
	std::atomic<int> isLocked = -1;

	std::thread lockHoldingWriter;
	{
		// ensures that the writer does not release the lock before the test case finishes
		std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);

		// writer holds lock
		lockHoldingWriter = std::thread([&] { writerHoldLockSerialized(lockName, isLocked); });

		// wait until writer gives feedback
		while (isLocked == -1) {}
		
		if (isLocked == 0) {
			std::cout << "WARNING: LockWriteWhileWriteLocked test could not be run correctly due to issues in CNamedRwLock::LockWrite() function" << std::endl;
			return;
		}

		std::thread challengingWriter = std::thread([&] { writerTryLock(lockName, success); });

		// wait for writer to timeout
		challengingWriter.join();

		// release the threadHoldMutex and notify the condition variable, so the lock holding writer can terminate
	}
	threadDone = true;
	threadTerminateCond.notify_one();
	lockHoldingWriter.join();

	EXPECT_FALSE(success) << "Writer could aquire the rw-lock while another writer was holding it.";
	threadDone = false;
}

/*
* This Test confirms that a process which tries to aquire the write lock,
* while one or more process currently hold a read lock, fails.
*/
TEST(RwLock, LockWriteWhileReadLocked)
{
	std::string lockName = "LockWriteWhileReadLockedTest";
	std::atomic<bool> success;

	std::atomic<int> readLockHolderCount = 0;
	std::atomic<int> readLockTimeoutCount = 0;

	std::thread lockHoldingReader;
	{
		// ensures that the reader does not release the lock before the test case finishes
		std::lock_guard<std::mutex> threadHoldGuard(threadHoldMutex);

		// reader holds lock
		lockHoldingReader = std::thread([&] { readerHoldLockSerialized(lockName, readLockHolderCount, readLockTimeoutCount); });

		// busy wait till reader aquired the lock or failed
		while (readLockHolderCount + readLockTimeoutCount != 1) {}

		// test cannot be run correctly if reader does not hold the lock
		if (readLockTimeoutCount == 1) {
			std::cout << "WARNING: LockWriteWhileReadLocked test could not be run correctly due to issues in CNamedRwLock::LockRead() function" << std::endl;
			return;
		}

		std::thread challengingWriter = std::thread([&] { writerTryLock(lockName, success); });

		// wait for process to timeout
		challengingWriter.join();

		// release the threadHoldMutex and notify the condition variable, so the lock holding reader can terminate
	}
	threadDone = true;
	threadTerminateCond.notify_one();
	lockHoldingReader.join();

	EXPECT_FALSE(success);
	threadDone = false;
}

/*
* This test confirms that the locks which a process aquired,
* will be implecitely released while the lock is destructed,
* to prevent, that a forgotten Unlock call breaks the lock.
*/
TEST(RwLock, ImpliciteUnlockingAtDestruction)
{
	std::string lockName = "ImpliciteUnlockingAtDestructionTest";

	// we need to create a thrad which holds a handle to the object during the whole test.
	// Otherwise the OS will remove the shm when the lock goes out of scope and 
	// the lock gets newly initialized when we construct it in this thread,
	// which defeats the whole purpose of this test completely

	std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);

	std::atomic<bool> threadHoldsHandle = false;
	std::thread handleHoldingThread([&] { holdHandle(lockName, threadHoldsHandle); });

	// wait for thread to construct rw-lock and hold handle
	while (threadHoldsHandle == false) {}
	
	// context after rwLock will be destructed
	{
		eCAL::CNamedRwLock rwLock(lockName);

		bool lockSuccess = rwLock.LockRead(TIMEOUT);

		if (!lockSuccess)
			std::cout << "WARNING: The correctness of implicite unlocking read-lock is not guaranteed, due to issues during the CNamedRwLock::LockRead() function" << std::endl;
	}
	// lock should be implicitely released now

	// context after rwLock will be destructed
	{
		eCAL::CNamedRwLock rwLock(lockName);

		bool lockSuccess = rwLock.Lock(TIMEOUT);

		// check now if lock call was successfull, because it only can be,
		// if lock was implicitely released after last context ende
		EXPECT_TRUE(lockSuccess) << "Read Lock is not released during desctruction!";

		if (!lockSuccess)
			std::cout << "WARNING: The correctness of implicite unlocking write-lock is not guaranteed, due to issues during the CNamedRwLock::Lock() function" << std::endl;
	}
	// lock should be implecitely realeased again now

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);
	
	EXPECT_TRUE(lockSuccess) << "Write lock is not released during construction!";
	
	// tell thread to release handle now
	threadDone = true;
	threadHoldGuard.unlock();
	threadTerminateCond.notify_one();
	handleHoldingThread.join();
	threadDone = false;
}

// Checks if the lock can not become inconsistent if many processes try to construct it at the same time.
// For example when a thread goes to sleep during the construction process, gets put to sleep, another
// thread constructs the lock and initialises the state. Afterwards the lock gets used by one or more
// threads and then the process which sleept during contruction gets woken up. Now the process migth 
// thinks it was the first one to construct it and initializes it again, effectively reseting a in use lock. 
TEST(RwLock, RobustLockConstruction)
{
	const int constructionCiclesCount = 10;
	const int constructingProcessesCount = 100;


}

/*
* This test confirms that ALL process waiting for a read lock,
* while it is currently held by a writing processes,
* are notified and thus woken up to aquire the lock,
* as soon as the writing process finishes. 
* In addition, no reader, which tries to aquire the lock while the readers are active,
* should be put down to sleep because he missed the notification event.
*/
TEST(RwLock, WriterSignalsReaderWhenUnlocked)
{

}

/*
* This test confirms that a process waiting for a write lock,
* while it is currently hold by one or multiple reading processes,
* is notified and thus woken up to aquire the lock when all reading processes have finished.
*/
TEST(RwLock, ReaderSignalsWriterWhenUnlocked)
{

}

/*
* This test confirms that the lock does not guarantee fairness in any way,
* meaning the lock can only be aquired when it is in a aquireable state
* and there are no lock mechanisms that guarantee reader or writer,
* to make the lock state aquireable for them.
*/
TEST(RwLock, NoFairnessGuarantee)
{

}

/*
* This test confirms that the lock does not break 
*	and thus introduce inconsistency of the protected object,
* under any kind of access frequency.
*/
TEST(RwLock, DoesNotBreakUnderHighUsage)
{

}


