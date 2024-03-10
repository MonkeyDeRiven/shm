#include <ecal_memfile.h>
#include <ecal_memfile_header.h>

#include "test_case.h"
#include "test_case_copy.h"
#include "test_case_zero_copy.h"
#include "test_case.pb.h"

#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <exception>

const int WRITE_ACCESS_TIMEOUT = 100;
const int READ_ACCESS_TIMEOUT = 100;
const int WRITE_ADDESS_TIMEOUT = 10;

const bool SEND_RAW_DATA = true;

const int WARMUP = 100;

//Create test cases list
std::vector<TestCaseZeroCopy> createTestCasesZeroCopy();
std::vector<TestCaseCopy> createTestCasesCopy();

//run tests
void runTests(std::string fileName, eCAL::CMemoryFile::lock_type lock_type);
void runTestsZeroCopy(std::vector<TestCaseZeroCopy>& testCases, std::string fileName, eCAL::CMemoryFile::lock_type lock_type);
void runTestsCopy(std::vector<TestCaseCopy>& testCases, std::string fileName, eCAL::CMemoryFile::lock_type lock_type);

//reader-writer thread creation
template<typename T>
std::thread createWriter(T& testCase, eCAL::CMemoryFile& memoryFile);

std::thread createReader(TestCase& testCase, eCAL::CMemoryFile& memoryFile, int timesIndex);

//reader-writer tasks
void writerTask(TestCase& testCase, eCAL::CMemoryFile& memoryFile);
void readerTaskZeroCopy(TestCaseZeroCopy& testCase, eCAL::CMemoryFile& memoryFile, int timesIndex);
void readerTaskCopy(TestCaseCopy& testCase, eCAL::CMemoryFile& mermoryFile, int timesIndex);

//time measurement
void saveTestResults(shm::Test_pb& testCase, std::string fileName);

//thread sync
std::condition_variable readerWriterSync;
std::mutex readerWriterLock;

//reader writer coordination
std::mutex readerDoneLock;
std::atomic<int> readerDoneCount(0);
std::atomic<int> writerDoneCount(0);
std::atomic<int> totalReaderCount(0);
std::atomic<bool> contentAvailable(false);

void readerDone()
{
	std::unique_lock <std::mutex> rdLock(readerDoneLock);
	readerDoneCount++;
	if (readerDoneCount == totalReaderCount) {
		contentAvailable = false;
		readerDoneCount = 0;
		readerWriterSync.notify_all();
	}
}

int main()
{
	std::string testResultFileName = "eCAL_base_lock_test";
	
	if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) 
	{
		std::cout << "This Process is running with realtime priority now!" << std::endl << std::endl;
	};

	//run tests with mutex lock
	runTests(testResultFileName, eCAL::CMemoryFile::lock_type::mutex);

	testResultFileName = "thesis_rw_lock_non_recoverable_test";

	//run tests with rw lock
	runTests(testResultFileName, eCAL::CMemoryFile::lock_type::rw_lock);

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	return 0;
}

std::vector<TestCaseZeroCopy> createTestCasesZeroCopy()
{
	auto testCases = std::vector<TestCaseZeroCopy>();

	// 1ms calculation time
	
	testCases.push_back(TestCaseZeroCopy(1, 1, 100, 1));
	testCases.push_back(TestCaseZeroCopy(3, 1, 100, 1));
	testCases.push_back(TestCaseZeroCopy(5, 1, 100, 1));
	testCases.push_back(TestCaseZeroCopy(10, 1, 100, 1));
	testCases.push_back(TestCaseZeroCopy(20, 1, 100, 1));
	
	// 10ms calculation time
	
	testCases.push_back(TestCaseZeroCopy(1, 1, 100, 100));
	testCases.push_back(TestCaseZeroCopy(3, 1, 100, 100));
	testCases.push_back(TestCaseZeroCopy(5, 1, 100, 100));
	testCases.push_back(TestCaseZeroCopy(10, 1, 100, 100));
	testCases.push_back(TestCaseZeroCopy(20, 1, 100, 100));
	
	// 100ms calculation time
	//testCases.push_back(TestCaseZeroCopy(1, 1, 1000, 50));
	//testCases.push_back(TestCaseZeroCopy(3, 1, 1000, 50));
	//testCases.push_back(TestCaseZeroCopy(5, 1, 1000, 50));
	//testCases.push_back(TestCaseZeroCopy(10, 1, 1000, 50));
	//testCases.push_back(TestCaseZeroCopy(20, 1, 1000, 10));
	//testCases.push_back(TestCaseZeroCopy(50, 1, 1000, 10));

	return testCases;
}

std::vector<TestCaseCopy> createTestCasesCopy()
{
	auto testCases = std::vector<TestCaseCopy>();

	// 10 Byte 
	
	testCases.push_back(TestCaseCopy(1, 1, 1000, 10));
	testCases.push_back(TestCaseCopy(3, 1, 1000, 10));
	testCases.push_back(TestCaseCopy(5, 1, 1000, 10));
	testCases.push_back(TestCaseCopy(10, 1, 1000, 10));
	testCases.push_back(TestCaseCopy(20, 1, 1000, 10));

	// 1 KB
	
	testCases.push_back(TestCaseCopy(1, 1, 1000, 1000));
	testCases.push_back(TestCaseCopy(3, 1, 1000, 1000));
	testCases.push_back(TestCaseCopy(5, 1, 1000, 1000));
	testCases.push_back(TestCaseCopy(10, 1, 1000, 1000));
	testCases.push_back(TestCaseCopy(20, 1, 1000, 1000));
	
	// 1 MB
	testCases.push_back(TestCaseCopy(1, 1, 1000, 1000000));
	testCases.push_back(TestCaseCopy(3, 1, 1000, 1000000));
	testCases.push_back(TestCaseCopy(5, 1, 1000, 1000000));
	testCases.push_back(TestCaseCopy(10, 1, 1000, 1000000));
	testCases.push_back(TestCaseCopy(20, 1, 1000, 1000000));

	// 50MB
	testCases.push_back(TestCaseCopy(1, 1, 1000, 50000000));
	testCases.push_back(TestCaseCopy(3, 1, 1000, 50000000));
	testCases.push_back(TestCaseCopy(5, 1, 1000, 50000000));
	testCases.push_back(TestCaseCopy(10, 1, 1000, 50000000));
	testCases.push_back(TestCaseCopy(20, 1, 1000, 50000000));
	
	return testCases;
}

void saveTestResults(shm::Test_pb& data, std::string fileName)
{
	std::ofstream file;
	file.open(fileName, std::ios::binary | std::ios::out);

	std::cout << "save results..." << std::endl;

	if (file.is_open()) {
		file << data.SerializeAsString();
		file.close();
		std::cout << "results saved!" << std::endl << std::endl;
	}
	else {
		std::cout << "ERROR: could not save test results!" << std::endl << std::endl;
	}
}

void runTests(std::string fileName, eCAL::CMemoryFile::lock_type lock_type) 
{
	//create test cases
	std::vector<TestCaseZeroCopy> testCasesZeroCopy = createTestCasesZeroCopy();
	std::vector<TestCaseCopy> testCasesCopy = createTestCasesCopy();

	//run tests
	runTestsZeroCopy(testCasesZeroCopy, fileName, lock_type);
	runTestsCopy(testCasesCopy, fileName, lock_type);

	// create results protobuf message for test
	shm::Test_pb message;
	for (int i = 0; i < testCasesZeroCopy.size(); i++) {
		*message.add_zerocopycases() = testCasesZeroCopy[i].getPbTestCaseMessage(true);
	}
	for (int i = 0; i < testCasesCopy.size(); i++) {
		*message.add_copycases() = testCasesCopy[i].getPbTestCaseMessage(true);
	}

	// save test results
	saveTestResults(message, fileName);

}

void runTestsZeroCopy(std::vector<TestCaseZeroCopy>& testCases, std::string fileName, eCAL::CMemoryFile::lock_type lock_type)
{
	std::ofstream file;

	std::cout << "run zero copy tests" << std::endl << std::endl;

	for (int i = 0; i < testCases.size(); i++) {

		TestCaseZeroCopy& testCase = testCases[i];

		std::cout << "test " << i + 1 << " in progress..." << std::endl;
		// Create memoryFile
		eCAL::CMemoryFile memoryFile(lock_type);
		memoryFile.Create("TestZeroCopy", true, testCase.getPayloadSize());

		// needed for reader writer coordination
		totalReaderCount = testCase.getSubCount();
		std::vector<std::thread> threadHandles;

		// create writer and push it to the thread handle vector
		threadHandles.push_back(createWriter(testCase, memoryFile));

		// create reader and push it to the thread handle vector
		for (int j = 0; j < testCase.getSubCount(); j++) {
			threadHandles.push_back(createReader(testCase, memoryFile, j));
		}

		// wait for test to finish
		for (int j = 0; j < threadHandles.size(); j++) {
			threadHandles[j].join();
		}

		std::cout << "test completed" << std::endl << std::endl;

		// evaluate test results
		testCase.calculateMetrics();

		// reset writer count for next test case
		writerDoneCount = 0;
	}
}

void runTestsCopy(std::vector<TestCaseCopy>& testCases, std::string fileName, eCAL::CMemoryFile::lock_type lock_type)
{
	std::cout << "run test copy" << std::endl << std::endl;

	for (int i = 0; i < testCases.size(); i++) {
		TestCaseCopy& testCase = testCases[i];

		std::cout << "Test " << i + 1 << " in progress..." << std::endl;

		//needed for reader writer coordination
		totalReaderCount = testCase.getSubCount();
		std::vector<std::thread> workers;

		//create memory file
		eCAL::CMemoryFile memoryFile(lock_type);
		memoryFile.Create("TestCopy", true, testCase.getPayloadSize());

		//add writer as first element
		workers.push_back(createWriter(testCase, memoryFile));

		//add readers
		for (int j = 0; j < testCase.getSubCount(); j++) {
			workers.push_back(createReader(testCase, memoryFile, j));
		}

		//wait for test to finish
		for (int j = 0; j < workers.size(); j++) {
			workers[j].join();
		}
		std::cout << "test completed" << std::endl << std::endl;
		testCase.calculateMetrics();
		writerDoneCount = 0;
	}
}

template<typename T>
std::thread createWriter(T& testCase, eCAL::CMemoryFile& memoryFile) {
	return std::thread([&]() {
		writerTask(testCase, memoryFile);
		});
}

std::thread createReader(TestCase& testCase, eCAL::CMemoryFile& memoryFile, int timesIndex)
{
	if (dynamic_cast<TestCaseZeroCopy*>(&testCase) != nullptr) {
		return std::thread([&testCase, &memoryFile, timesIndex]() {
			readerTaskZeroCopy(dynamic_cast<TestCaseZeroCopy&>(testCase), memoryFile, timesIndex);
			});
	}
	return std::thread([&testCase, &memoryFile, timesIndex]() {
		readerTaskCopy(dynamic_cast<TestCaseCopy&>(testCase), memoryFile, timesIndex);
		});
}

void writerTask(TestCase& testCase, eCAL::CMemoryFile& memoryFile)
{
	auto beforeAccess = std::chrono::steady_clock::now().time_since_epoch();
	auto afterAccess = std::chrono::steady_clock::now().time_since_epoch();
	auto afterRelease = std::chrono::steady_clock::now().time_since_epoch();
	for (int i = 0; i < testCase.getMsgCount()+WARMUP; i++) {
		if (contentAvailable == false) {
			//blocks the reader till content is written
			std::unique_lock<std::mutex> w_lock(readerWriterLock);

			beforeAccess = std::chrono::steady_clock::now().time_since_epoch();
			while (!memoryFile.GetWriteAccess(WRITE_ACCESS_TIMEOUT)) {}
			afterAccess = std::chrono::steady_clock::now().time_since_epoch();

			memoryFile.WriteBuffer(testCase.getPayload().get()->data(), testCase.getPayloadSize(), 0);
			memoryFile.ReleaseWriteAccess();
			afterRelease = std::chrono::steady_clock::now().time_since_epoch();
			contentAvailable = true;
			writerDoneCount++;
			//allows readers to read written content
			readerWriterSync.notify_all();

			//process taken time while readers can read
			if (i >= WARMUP) {
				testCase.pushToPubBeforeAccessTimes(std::chrono::duration_cast<std::chrono::microseconds>(beforeAccess).count());
				testCase.pushToPubAfterAccessTimes(std::chrono::duration_cast<std::chrono::microseconds>(afterAccess).count());
				testCase.pushToPubAfterReleaseTimes(std::chrono::duration_cast<std::chrono::microseconds>(afterRelease).count());
			}
			readerWriterSync.wait(w_lock, [] { return !contentAvailable; });
		}
	}
}

void readerTaskZeroCopy(TestCaseZeroCopy& testCase, eCAL::CMemoryFile& memoryFile, int timesIndex)
{
	std::vector<std::string> _buf(POINTER_SIZE);
	auto beforeAccess = std::chrono::steady_clock::now().time_since_epoch();
	auto afterAccess = std::chrono::steady_clock::now().time_since_epoch();
	auto afterRelease = std::chrono::steady_clock::now().time_since_epoch();
	for (int i = 0; i < testCase.getMsgCount()+WARMUP; i++) {
		//wait till content is available
		std::unique_lock<std::mutex> r_lock(readerWriterLock);
		readerWriterSync.wait(r_lock, [=] { return writerDoneCount == i + 1; });
		r_lock.unlock();
		//aquire read access, could already be aquired by different reader
		beforeAccess = std::chrono::steady_clock::now().time_since_epoch();
		while (!memoryFile.GetReadAccess(READ_ACCESS_TIMEOUT)) {}
		afterAccess = std::chrono::steady_clock::now().time_since_epoch();

		memoryFile.Read(_buf.data(), testCase.getPayloadSize(), 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(testCase.getCalculationTime()));
		memoryFile.ReleaseReadAccess();
		afterRelease = std::chrono::steady_clock::now().time_since_epoch();
		readerDone();
		if (i >= WARMUP) {
			testCase.pushToSubBeforeAccessTimes(std::chrono::duration_cast<std::chrono::microseconds>(beforeAccess).count(), timesIndex);
			testCase.pushToSubAfterAccessTimes(std::chrono::duration_cast<std::chrono::microseconds>(afterAccess).count(), timesIndex);
			testCase.pushToSubAfterReleaseTimes(std::chrono::duration_cast<std::chrono::microseconds>(afterRelease).count(), timesIndex);
		}
	}
}

void readerTaskCopy(TestCaseCopy& testCase, eCAL::CMemoryFile& memoryFile, int timesIndex)
{
	std::vector<char> _buf = std::vector<char>(testCase.getPayloadSize());
	auto beforeAccess = std::chrono::steady_clock::now().time_since_epoch();
	auto afterAccess = std::chrono::steady_clock::now().time_since_epoch();
	auto afterRelease = std::chrono::steady_clock::now().time_since_epoch();
	for (int i = 0; i < testCase.getMsgCount()+WARMUP; i++) {
		//wait till content is available
		std::unique_lock<std::mutex> r_lock(readerWriterLock);
		readerWriterSync.wait(r_lock, [=]() { return writerDoneCount == i + 1; });

		//unlock so other readers are not blocked
		r_lock.unlock();

		//aquire read access, could already be aquired by different reader
		while (!memoryFile.GetReadAccess(READ_ACCESS_TIMEOUT)) {}
		afterAccess = std::chrono::steady_clock::now().time_since_epoch();

		memoryFile.Read(_buf.data(), testCase.getPayloadSize(), 0);
		memoryFile.ReleaseReadAccess();

		afterRelease = std::chrono::steady_clock::now().time_since_epoch();

		readerDone();

		if (i >= WARMUP) {
			testCase.pushToSubBeforeAccessTimes(std::chrono::duration_cast<std::chrono::microseconds>(beforeAccess).count(), timesIndex);
			testCase.pushToSubAfterAccessTimes(std::chrono::duration_cast<std::chrono::microseconds>(afterAccess).count(), timesIndex);
			testCase.pushToSubAfterReleaseTimes(std::chrono::duration_cast<std::chrono::microseconds>(afterRelease).count(), timesIndex);
		}
	}
}