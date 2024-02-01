#include "gtest/gtest.h"
#include "io/rw-lock/ecal_named_rw_lock.h"

#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

// assures that the thread access the rw-lock in the order that the test expects it
std::mutex serializeMutex;

// enables a thread to hold a lock till it is not needed anymore
std::condition_variable threadTerminateCond;
std::mutex threadHoldMutex;
bool threadDone = false;

// timeout for the rw-lock operations
const int64_t TIMEOUT = 20;

void writerHoldLock(std::string& lockName)
{
	// serialize the lock aquisition to avoid unexpected timeouts during test proccess 
	std::unique_lock <std::mutex> serializeGuard(serializeMutex);
	
	// create the rw-lock
	eCAL::CNamedRwLock rwLock = eCAL::CNamedRwLock(lockName);
	rwLock.Lock(TIMEOUT);

	// release the serialize lock now that the writer holds the lock
	serializeGuard.unlock();

	// hold the rw-lock until notification arrives
	threadTerminateCond.wait(std::unique_lock<std::mutex>(threadHoldMutex), []{ return threadDone;});
	
	// serialize the lock release to avoid unexpected timeouts during test process
	serializeGuard.lock();
	rwLock.Unlock();
	serializeGuard.unlock();
}

void readerHoldLock(std::string& lockName)
{
	// serialize the lock aquisition to avoid unexpected timeouts during test proccess 
	std::unique_lock <std::mutex> serializeGuard(serializeMutex);

	// create the rw-lock
	eCAL::CNamedRwLock rwLock = eCAL::CNamedRwLock(lockName);
	rwLock.LockRead(TIMEOUT);

	// release the serialize lock now that the writer holds the lock
	serializeGuard.unlock();

	// hold the rw-lock and dont release it
	threadTerminateCond.wait(std::unique_lock<std::mutex>(threadHoldMutex), [] { return threadDone; });

	// serialize the lock release to avoid unexpected timeouts during test process
	serializeGuard.lock();
	rwLock.Unlock();
	serializeGuard.unlock();
}

void writerTryLock(std::string& lockName, bool& success)
{
	// create the rw-lock
	eCAL::CNamedRwLock rwLock(lockName);

	{
		// wait for anyone that needs to adjust the lock state first
		std::lock_guard<std::mutex> serializeGuard(serializeMutex);
	}
	// try to lock the file	within timeout period and write the result to sucess
	success = rwLock.Lock(TIMEOUT);
}

TEST(RwLock, LockWriteTimeout)
{
	std::string lockName = "LockWriteTest";
	bool success;
	std::thread firstWriter;
	{
		// ensures that the writer does not release the lock before the test case finishes
		std::unique_lock<std::mutex> threadHoldGuard(threadHoldMutex);

		// writer holds lock
		firstWriter = std::thread(&writerHoldLock, lockName);

		// make sure that firstWriter can aquire the serializeMutex before second writer tries to aquire the lock
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		std::thread challengingWriter = std::thread([&] { writerTryLock(lockName, success); });
		
		// wait for writer to timeout
		challengingWriter.join();

		threadDone = true;
		// release the threadHoldMutex and notify the condition variable, so the lock holding writer can terminate
	}
	threadTerminateCond.notify_one();
	firstWriter.join();
	threadDone = false;

	EXPECT_FALSE(success) << "Writer could aquire the rw-lock while another writer was holding it.";

	std::thread firstReader;
	{
		// ensures that the reader does not release the lock before the test case finishes
		std::lock_guard<std::mutex> threadHoldGuard(threadHoldMutex);

		// reader holds lock
		firstReader = std::thread([&] { readerHoldLock(lockName); });

		// make sure that firstReader can aquire the serializeMutex before second writer tries to aquire the lock
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		std::thread challengingWriter = std::thread([&] { writerTryLock(lockName, success); });

		// wait for process to timeout
		challengingWriter.join();

		threadDone = true;
		// release the threadHoldMutex and notify the condition variable, so the lock holding reader can terminate
	}
	threadTerminateCond.notify_one();
	firstReader.join();
	threadDone = false;

	EXPECT_FALSE(success);
	
}

TEST(RwLock, UnlockWrite)
{
	
	
}

TEST(RwLock, LockRead)
{

}

TEST(RwLock, UnlockRead)
{

}

TEST(RwLock, WriterSignalsReader)
{

}

TEST(RwLock, ReaderSignalsWriter) 
{


}


