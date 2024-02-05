#include "gtest/gtest.h"
#include "io/rw-lock/ecal_named_rw_lock.h"

#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <condition_variable>

#include <Windows.h>

// enables a thread to hold a lock till it is not needed anymore
std::condition_variable threadTerminateCond;
std::mutex threadHoldMutex;
std::atomic<bool> threadDone = false;

// timeout for the rw-lock operations
const int64_t TIMEOUT = 200;

void writerHoldLockSerialized(const std::string& lockName, std::atomic<int>& feedback /* -1=not signaled, 0=fail, 1=success*/)
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

void readerHoldLockSerialized(const std::string& lockName, std::atomic<int>& lockHolderCount, std::atomic<int>& lockTimeoutCounter)
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

void writerTryLock(const std::string& lockName, std::atomic<bool>& success)
{
	// create the rw-lock
	eCAL::CNamedRwLock rwLock(lockName);

	success = rwLock.Lock(TIMEOUT);

	if (success == true) {
		rwLock.Unlock();
	}
}

void readerTryLock(const std::string& lockName, std::atomic<bool>& success)
{
	eCAL::CNamedRwLock rwLock(lockName);
	// try to lock the file	within timeout period and write the result to success
	success = rwLock.LockRead(TIMEOUT);

	if (success == true) {
		rwLock.UnlockRead(TIMEOUT);
	}
}

void holdHandle(const std::string& lockName, std::atomic<bool>& holdingHandle)
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
	const std::string lockName = "LockWriteTest";

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
	const std::string lockName = "LockReadTest";

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
	const std::string lockName = "UnlockWriteTest";

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
	const std::string lockName = "UnlockReadTest";

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
	const std::string lockName = "UnlockReadByNoneReadLockHolderTest";

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

	EXPECT_EQ(1, rwLock.GetReaderCount()) << "Read lock was unlocked by non read lock holder!";

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
	const std::string lockName = "UnlockWriteByNoneWriteLockHolderTest";

	// feedback to see if writer could aquire lock
	// -1 -> no feedback yet
	//  0 -> failed
	//  1 -> success
	std::atomic<int> isLocked = -1;
	
	std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);

	std::thread lockHoldingWriter([&] { writerHoldLockSerialized(lockName, isLocked); });

	while (isLocked == -1) {};

	if (isLocked == 0) {
		std::cout << "WARNING: LockReadWhileWriteLocked test could not be run correctly due to issues in CNamedRwLock::LockWrite() function" << std::endl;
		return;
	}

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Unlock();

	EXPECT_FALSE(lockSuccess);

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
	const std::string lockName = "LockReadWileReadLockedTest";

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
	const std::string lockName = "LockReadWhileReadLockedTest";
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
	const std::string lockName = "LockWriteWhileWriteLockedTest";
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
	const std::string lockName = "LockWriteWhileReadLockedTest";
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
	const std::string lockName = "ImpliciteUnlockingAtDestructionTest";

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

void useWriteLock(const std::string& lockName, int& sharedCounter, const int& incrementCount) 
{
	eCAL::CNamedRwLock rwLock(lockName);

	if (rwLock.Lock(INFINITE)) {
		for (int i = 0; i < incrementCount; i++)
			sharedCounter++;
		rwLock.Unlock();
	}
}

void useReadLock(const std::string& lockName, int& sharedCounter, const int& readCicleCount, std::vector<bool>& readConsistentResults, int& index)
{
	eCAL::CNamedRwLock rwLock(lockName);
	if (rwLock.LockRead(TIMEOUT)) {
		// make a copy of shared counter value
		int counterValue = sharedCounter;
		for (int i = 0; i < readCicleCount; i++) {
			// check if the shared Counter value changes
			if (counterValue != sharedCounter)
				// if the shared counter changes the value was modified while a read lock was held
				readConsistentResults[index] = false;
		}
		rwLock.UnlockRead(TIMEOUT);
	}
}

#include <chrono>
// Checks if the lock can not become inconsistent if many processes try to construct it at the same time.
// For example when a thread goes to sleep during the construction process, gets put to sleep, another
// thread constructs the lock and initialises the state. Afterwards the lock gets used by one or more
// threads and then the process which sleept during contruction gets woken up. Now the process migth 
// thinks it was the first one to construct it and initializes it again, effectively reseting a in use lock. 
TEST(RwLock, RobustLockConstruction)
{
	int testDurationMs = 1000;
	std::cout << "INFO: The test duration is set to " << testDurationMs / 1000 << "sec" << std::endl;
	const std::string lockName = "RobustLockConstructionTest";
	//const int constructionCiclesCount = 3000;
	const int constructingProcessesCount = 2;
	const int incrementSharedCounterCount = 10;

	std::vector<std::thread> threadHandles;

	// start time in ms
	auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	auto endTime = startTime;
	//for (int i = 0; i < constructionCiclesCount; i++) {
	while(endTime - startTime < testDurationMs){
		int sharedCounter = 0;
		std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);
		for (int i = 0; i < constructingProcessesCount; i++) {
			threadHandles.push_back(std::thread([&] { useWriteLock(lockName, sharedCounter, incrementSharedCounterCount); }));
		}
		for (auto& thread : threadHandles) {
			thread.join();
		}
		EXPECT_EQ(constructingProcessesCount * incrementSharedCounterCount, sharedCounter);
		threadHandles.clear();
		endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

}

/*
* This test confirms that ALL process waiting for a read lock,
* while it is currently held by a writing processes,
* are notified and thus woken up to aquire the lock,
* as soon as the writing process finishes. 
* In addition, no reader, which tries to aquire the lock while the readers are active,
* should be put down to sleep because it missed the notification event.
*/
TEST(RwLock, WriterSignalsReaderWhenUnlocked)
{
	std::string lockName = "WriterSignalsReaderWhenUnlockedTest";
	int waitingReaderCount = 1000;

	// lock holding writer feedback
	// -1 -> no feedback yet
	//  0 -> failed
	//  1 -> success
	std::atomic<int> isLocked = -1;

	std::atomic<bool> success = false;

	// waiting reader feedback
	std::atomic<int> readLockHolderCount = 0;
	std::atomic<int> readLockTimeoutCount = 0;

	std::thread lockHoldingWriter;
	std::vector<std::thread> waitingReaders;

	// reader which misses the initial event
	std::thread lateReader;
	// this writer competes against the late reader for the lock access
	// expected behavior is that the writer should get the access first everytime
	// because the late reader tries to access the lock while other readers have it
	// and thus the lock should not refuse the valid access of the late reader
	std::thread competingWriter;
	{
		std::unique_lock<std::mutex> threadDoneGuard(threadHoldMutex);

		// writer holds lock
		lockHoldingWriter = std::thread([&] { writerHoldLockSerialized(lockName, isLocked); });

		// busy wait till writer aquired the lock or failed
		while (isLocked == -1) {};

		// test cannot be run correctly if reader does not hold the lock
		if (isLocked == 0) {
			std::cout << "WARNING: WriterSignalsReaderWhenUnlocked test could not be run correctly due to issues in CNamedRwLock::LockWrite() function" << std::endl;
			return;
		}

		// create waiting reader
		for (int i = 0; i < waitingReaderCount; i++) {
			waitingReaders.push_back(std::thread([&] { readerTryLock(lockName, success); }));
		}
	}
	// make writer release the lock
	threadDone = true;
	threadTerminateCond.notify_all();
	lockHoldingWriter.join();
	threadDone = false;
	// write lock is now released and waiting readers are working
	{
		std::unique_lock<std::mutex> threadDoneGuard(threadHoldMutex);
		// time to spawn a late Reader and see if he beats the writer for the access
		lateReader = std::thread([&] { readerHoldLockSerialized(lockName, readLockHolderCount, readLockTimeoutCount); });
		// now spawn the competing writer
		competingWriter = std::thread([&] { writerHoldLockSerialized(lockName, isLocked); });

		for (auto& thread : waitingReaders) {
			thread.join();
		}

		// the winner of the access competition should now hold the lock,
		// while the loser waits for the lock so we check if the reader won
		EXPECT_EQ(1, readLockHolderCount);
	}

	threadDone = true;
	threadTerminateCond.notify_all();
	lateReader.join();
	competingWriter.join();
	threadDone = false;

}

/*
* This test confirms that a process waiting for a write lock,
* while it is currently hold by one or multiple reading processes,
* is notified and thus woken up to aquire the lock when all reading processes have finished.
*/
TEST(RwLock, ReaderSignalsWriterWhenUnlocked)
{
	std::string lockName = "ReaderSignalsWriterWhenUnlockedTest";

	// lock holding reader feedback
	std::atomic<int> readLockHolderCount = 0;
	std::atomic<int> readLockTimeoutCount = 0;

	// waiting writer feedback
	// -1 -> no feedback yet
	//  0 -> failed
	//  1 -> success
	std::atomic<int> isLocked = -1;

	std::thread lockHoldingWriter;
	std::thread waitingWriter;
	{
		std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);

		// writer holds lock
		lockHoldingWriter = std::thread([&] { readerHoldLockSerialized(lockName, readLockHolderCount, readLockTimeoutCount); });

		// busy wait till reader aquired the lock or failed
		while (readLockHolderCount + readLockTimeoutCount != 1) {}

		// test cannot be run correctly if reader does not hold the lock
		if (readLockTimeoutCount == 1) {
			std::cout << "WARNING: LockWriteWhileReadLocked test could not be run correctly due to issues in CNamedRwLock::LockRead() function" << std::endl;
			return;
		}

		// we need a writer which waits for the lock
		waitingWriter = std::thread([&] { writerHoldLockSerialized(lockName, isLocked); });

		// make sure that the writer is really waiting
	  // not a 100% guarantee
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	threadDone = true;
	threadTerminateCond.notify_all();
	lockHoldingWriter.join();
	waitingWriter.join();
	threadDone = false;

	EXPECT_TRUE(isLocked);
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

void runRobustnessTest(std::string lockName, int writerCount, int readerCount, int iterations, int readWriteCicles) 
{
	// the counter which should not become inconsistent during the lock usage
	int sharedCounter = 0;

	std::vector<std::thread> readerThreads;
	// the amount of times a reader shold check the counter
	const int readCicleCount = 5;
	std::vector<std::thread> writerThreads;
	// the amount of times a writer should increment the Counter
	int incrementCount = 10;

	// every reader gets a vector entry for its consistency result
	std::vector<bool> readConsistentResults(readerCount, true);

	for (int i = 0; i < iterations; i++) {
		for (int j = 0; j < readWriteCicles; j++) {
			std::cout << "iteration: " << i << " " << "cicle: " << j << std::flush;
			std::cout << '\r';
			// create writers
			for (int k = 0; k < writerCount; k++)
				writerThreads.push_back(std::thread([&] { useWriteLock(lockName, sharedCounter, incrementCount); }));
			// create readers
			for (int k = 0; k < readerCount; k++)
				readerThreads.push_back(std::thread([&] { useReadLock(lockName, sharedCounter, readCicleCount, readConsistentResults, k); }));
			// wait for writers
			for (auto& thread : writerThreads)
				thread.join();
			// wait for readers
			for (auto& thread : readerThreads)
				thread.join();
			// check if all read accesses where consistent
			for (auto consistent : readConsistentResults)
				ASSERT_TRUE(consistent) << "Writer was active during a read access";
			// check if shared counter became inconsistent
			int expectedCounterValue = writerCount * incrementCount;
			ASSERT_EQ(expectedCounterValue, sharedCounter) << "Two or more write accesses must have happened at the same time";
			// clear all handles
			readerThreads.clear();
			writerThreads.clear();
			// reset shared counter
			sharedCounter = 0;
		}
	}
}

/*
* The following tests confirmthat the lock does not break 
*	and thus introduce inconsistency of the protected object,
* under any kind of access frequency.
*/
TEST(RwLock, DoesNotBreakUnderHighReadUsage)
{
	const std::string lockName = "DoesNotBreakUnderHighReadUsageTest";

	int readerCount = 2000; 
	int writerCount = 10;

	int iterations = 10;
	int readWriteCicles = 20;

	std::cout << "this test will run for " << iterations << " iterations with " << readWriteCicles << " read/write cicles." << std::endl;
	runRobustnessTest(lockName, writerCount, readerCount, iterations, readWriteCicles);

}

TEST(RwLock, DoesNotBreakUnderHighWriteUsage)
{
	const std::string lockName = "DoesNotBreakUnderHighWriteUsageTest";

	int readerCount = 10;
	int writerCount = 1000;

	int iterations = 10;
	int readWriteCicles = 20;

	std::cout << "this test will run for " << iterations << "iterations with " << readWriteCicles << " read/write cicles." << std::endl;
	runRobustnessTest(lockName, writerCount, readerCount, iterations, readWriteCicles);

}

TEST(RwLock, DoesNotBreakUnderHighReadAndWriteUsage)
{
	const std::string lockName = "DoesNotBreakUnderHighReadUsageTest";

	int readerCount = 500;
	int writerCount = 500;

	int iterations = 10;
	int readWriteCicles = 20;

	std::cout << "this test will run for " << iterations << " with " << readWriteCicles << "read/write cicles." << std::endl;
	runRobustnessTest(lockName, writerCount, readerCount, iterations, readWriteCicles);
}



