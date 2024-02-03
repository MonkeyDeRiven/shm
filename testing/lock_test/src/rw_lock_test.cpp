#include "gtest/gtest.h"
#include "io/rw-lock/ecal_named_rw_lock.h"

#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <condition_variable>

// assures that the thread access the rw-lock in the order that the test expects it
std::mutex serializeMutex;

// enables a thread to hold a lock till it is not needed anymore
std::condition_variable threadTerminateCond;
std::mutex threadHoldMutex;
std::atomic<bool> threadDone = false;

// timeout for the rw-lock operations
const int64_t TIMEOUT = 20;

void writerHoldLockSerialized(std::string& lockName, int& feedback /* -1=not signaled, 0=fail, 1=success*/)
{
	// serialize the lock aquisition to avoid unexpected timeouts during test proccess 
	std::unique_lock <std::mutex> serializeGuard(serializeMutex);
	
	// create the rw-lock
	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);

	std::cout << "holding writer did call lock and the return value is " << lockSuccess << std::endl;

	if (lockSuccess)
		// signal success to main thread
		feedback = 1;
	else {
		// signal fail to main tread and exit since lock can not be held anyways
		feedback = 0;
		return;
	}

	// release the serialize lock now that the writer holds the lock
	serializeGuard.unlock();

	// hold the rw-lock until notification arrives
	threadTerminateCond.wait(std::unique_lock<std::mutex>(threadHoldMutex), []{ return threadDone.load();});
	
	// serialize the lock release to avoid unexpected timeouts during test process
	serializeGuard.lock();
	rwLock.Unlock();
	std::cout << "holding writer did call unlock!" << std::endl;
	serializeGuard.unlock();
}

// this function should always be able to aquire the read lock, since no other thread can try to aquire the lock at the same time.
// CAUTION! The guarantee above only applies, if this function does not get called in the same test with readerHoldLockParallel()!
void readerHoldLockSerialized(std::string& lockName, int& lockHolderCount, int& lockTimeoutCounter)
{
	// serialize the lock aquisition to avoid unexpected timeouts during test proccess 
	std::unique_lock <std::mutex> serializeGuard(serializeMutex);

	// create the rw-lock
	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.LockRead(TIMEOUT);

	std::cout << "holding writer did call lock and the return value is " << lockSuccess << std::endl;

	// release the serialize lock now that the writer holds the lock
	serializeGuard.unlock();

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

	// serialize the lock release to avoid unexpected timeouts during test process
	serializeGuard.lock();
	rwLock.Unlock();
	std::cout << "holding reader did call unlock!" << std::endl;
	serializeGuard.unlock();
}

void writerTryLock(std::string& lockName, bool& success)
{
	// create the rw-lock
	eCAL::CNamedRwLock rwLock(lockName);

	{
		// wait for anyone that needs to adjust the lock state first
		std::lock_guard<std::mutex> serializeGuard(serializeMutex);
		// try to lock the file	within timeout period and write the result to success
		success = rwLock.Lock(TIMEOUT);
		std::cout << "Writer challanged and the return value was " << success << std::endl;
	}
	
	if (success == true){
		rwLock.Unlock();
	}
}

void readerTryLock(std::string& lockName, bool& success)
{
	eCAL::CNamedRwLock rwLock(lockName);

	{
		// wait for anyone that needs to adjust the lock state first
		std::lock_guard<std::mutex> serializeGuard(serializeMutex);
		// try to lock the file	within timeout period and write the result to success
		success = rwLock.LockRead(TIMEOUT);
		std::cout << "Reader challanged and the return value was " << success << std::endl;
	}

	

	if (success == true) {
		rwLock.UnlockRead(TIMEOUT);
	}
}

TEST(RwLock, LockWrite) 
{
	std::string lockName = "LockWriteTest";

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);

	if (lockSuccess)
		rwLock.Unlock();

	EXPECT_TRUE(lockSuccess) << "The rw-lock denied write access, even tho no other process was holding the lock at the time.";
}

TEST(RwLock, LockRead) 
{
	std::string lockName = "LockReadTest";

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.LockRead(TIMEOUT);

	if (lockSuccess)
		rwLock.UnlockRead(TIMEOUT);

	EXPECT_TRUE(lockSuccess) << "The rw-lock denied read access, even tho no other process was holding the lock at the time.";
}

// edit this test as soon as Unlock() returns a bool as well
TEST(RwLock, UnlockWrite)
{
	std::string lockName = "UnlockWriteTest";

	eCAL::CNamedRwLock rwLock(lockName);
	bool lockSuccess = rwLock.Lock(TIMEOUT);

	if (lockSuccess == true) {
		rwLock.Unlock();
		lockSuccess = rwLock.Lock(TIMEOUT);
		EXPECT_TRUE(lockSuccess);
	}
	else
		std::cout << "WARNING: UnlockWrite test could not be run correctly due to error in CNamedRwLock::Lock() function" << std::endl;
}

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


TEST(RwLock, LockReadWhileReadLocked)
{
	std::string lockName = "LockReadWileReadLockedTest";

	int parallelReaderCount = 10;
	int readLockHolderCount = 0;
	int readLockTimeoutCount = 0;

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

TEST(RwLock, LockReadWhileWriteLocked)
{
	std::string lockName = "LockReadWhileReadLockedTest";
	bool success;

	// feedback to see if writer could aquire lock
	// -1 -> no feedback yet
	//  0 -> failed
	//  1 -> success
	int isLocked = -1;

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

TEST(RwLock, LockWriteWhileWriteLocked)
{
	std::string lockName = "LockWriteWhileWriteLockedTest";
	bool success;

	// feedback to see if writer could aquire lock
	// -1 -> no feedback yet
	//  0 -> failed
	//  1 -> success
	int isLocked = -1;

	std::thread lockHoldingWriter;
	{
		// ensures that the writer does not release the lock before the test case finishes
		std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);

		// writer holds lock
		lockHoldingWriter = std::thread([&] { writerHoldLockSerialized(lockName, isLocked); });

		// wait until writer gives feedback
		while(isLocked == -1) {}

		std::cout << "Holding writer signaled " << isLocked << std::endl;

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

TEST(RwLock, LockWriteWhileReadLocked) 
{
	std::string lockName = "LockWriteWhileReadLockedTest";
	bool success;

	int readLockHolderCount = 0;
	int readLockTimeoutCount = 0;

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


TEST(RwLock, ImpliciteUnlockingAtDestruction)
{

}

// Checks if the lock can not become inconsistent if many processes try to construct it at the same time
TEST(RwLock, RobustLockConstruction)
{

}


TEST(RwLock, WriterSignalsReaderWhenUnlocked)
{

}

TEST(RwLock, ReaderSignalsWriterWhenUnlocked) 
{

}

TEST(RwLock, NoFairnessGuarantee)
{

}


