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

#include <numeric>
#include <algorithm>

const int WRITE_ACCESS_TIMEOUT = 100;
const int READ_ACCESS_TIMEOUT = 100;
const int WRITE_ADDESS_TIMEOUT = 10;

const bool SEND_RAW_DATA = true;

const int WARMUP = 1000;

void printRateTestMetrics(std::vector<TestCaseCopy> rateTests);

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
std::atomic<int> readerDoneCount = 0;
std::atomic<int> writerDoneCount = 0;
std::atomic<int> totalReaderCount = 0;
std::atomic<bool> contentAvailable = false;

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

struct rateTest {
	eCAL::CMemoryFile::lock_type type;
	int subCount;
	// time each iteration took
	std::vector<long long> times;
	float avgTime;
	long long maxTime;
	long long minTime;
};

std::vector<rateTest> rateTests;


void runLockPerformanceCheck(eCAL::CMemoryFile::lock_type lockType, int iterations)
{
	const int TIMEOUT = 10;
	
	if (lockType == eCAL::CMemoryFile::lock_type::mutex) {
		eCAL::CNamedMutex mutex("some name", false);

		auto start = std::chrono::steady_clock::now().time_since_epoch();
		for (int i = 0; i < iterations; i++)
		{
			mutex.Lock(TIMEOUT);
			mutex.Unlock();
		}
		auto end = std::chrono::steady_clock::now().time_since_epoch();
		auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		std::cout << "duration: " << static_cast<double>(totalTime) / static_cast<double>(iterations) << "ms" << std::endl;
	}
	else {
		eCAL::CNamedRwLock rwLock("some name", false);

		auto start = std::chrono::steady_clock::now().time_since_epoch();
		for (int i = 0; i < iterations; i++)
		{
			rwLock.Lock(TIMEOUT);
			rwLock.Unlock();
		}
		auto end = std::chrono::steady_clock::now().time_since_epoch();
		auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		std::cout << "wlock duration: " << static_cast<double>(totalTime) / static_cast<double>(iterations) << "ms" << std::endl;

		start = std::chrono::steady_clock::now().time_since_epoch();
		for (int i = 0; i < iterations; i++)
		{
			rwLock.LockRead(TIMEOUT);
			rwLock.UnlockRead(TIMEOUT);
		}
		end = std::chrono::steady_clock::now().time_since_epoch();
		totalTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		std::cout << "rlock duration: " << static_cast<double>(totalTime) / static_cast<double>(iterations) << "ms" << std::endl << std::endl;
	}
}

void lockUnlockWrite(int& iterations)
{
	eCAL::CNamedRwLock rwLock("some name", false);

	auto start = std::chrono::steady_clock::now().time_since_epoch();
	for (int i = 0; i < iterations; i++)
	{
		rwLock.Lock(WRITE_ACCESS_TIMEOUT);
		rwLock.Unlock();
	}
	auto end = std::chrono::steady_clock::now().time_since_epoch();
	auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << "wlock duration: " << static_cast<double>(totalTime) / static_cast<double>(iterations) << "ms" << std::endl;

}
void lockUnlockWriteMu(int& iterations)
{
	eCAL::CNamedMutex rwLock("some name", false);

	auto start = std::chrono::steady_clock::now().time_since_epoch();
	for (int i = 0; i < iterations; i++)
	{
		rwLock.Lock(WRITE_ACCESS_TIMEOUT);
		rwLock.Unlock();
	}
	auto end = std::chrono::steady_clock::now().time_since_epoch();
	auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << "wlock duration: " << static_cast<double>(totalTime) / static_cast<double>(iterations) << "ms" << std::endl;

}

std::mutex outputLock;
void lockUnlockRead(int& iterations)
{
	eCAL::CNamedRwLock rwLock("some name", false);
	auto start = std::chrono::steady_clock::now().time_since_epoch();
	for (int i = 0; i < iterations; i++)
	{
		rwLock.LockRead(READ_ACCESS_TIMEOUT);
		rwLock.UnlockRead(READ_ACCESS_TIMEOUT);
	}
	auto end = std::chrono::steady_clock::now().time_since_epoch();
	auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	outputLock.lock();
	std::cout << "rlock duration: " << static_cast<double>(totalTime) / static_cast<double>(iterations) << "ms" << std::endl;
	outputLock.unlock();
}


void runRwLockPerformance(int iterations, int subCount)
{
	std::vector<std::thread> subs;
	for (int i = 0; i < subCount; i++){
		subs.push_back(std::thread([&] { lockUnlockRead(iterations); }));
	}
	for (auto& sub : subs){
		sub.join();
	}
}


std::vector<float> muavgs;
std::vector<long long> mumaxs;
std::vector<long long> mumins;
std::vector<float> rwavgs;
std::vector<long long> rwmaxs;
std::vector<long long> rwmins;

bool rwLock = false;

void saveLockData(std::map<int, std::vector<rateTest>> &mutexTests, std::map<int, std::vector<rateTest>> &rwLockTests)
{
	std::ofstream file;
	std::string fileName = "lock_rate_data_.txt";
	file.open(fileName);
	if (file.is_open()) {
		auto itMutex = mutexTests.begin();
		auto itRwLock = rwLockTests.begin();

		while (itMutex != mutexTests.end() && itRwLock != rwLockTests.end())
		{
			file << "subs:" << itMutex->first << "\n";
			for (int i = 0; i < itMutex->second.size(); i++) {
				file << itMutex->second[i].avgTime << "," << itMutex->second[i].minTime << "," << itMutex->second[i].maxTime;
				if (i < itMutex->second.size()-1) {
					file << ";";
				}
			}
			file << "\n";
			for (int i = 0; i < itRwLock->second.size(); i++) {
				file << itRwLock->second[i].avgTime << "," << itRwLock->second[i].minTime << "," << itRwLock->second[i].maxTime;
				if (i < itRwLock->second.size() - 1) {
					file << ";";
				}
			}
			file << "\n";
			++itMutex;
			++itRwLock;
		}
	}
	else {
		std::cout << "could not write to file" << std::endl << std::endl;
	}
	file.close();
}
#include "math.h"

void fillTestMap(std::map<int, std::vector<rateTest>>&mutexTests, std::map<int, std::vector<rateTest>>&rwLockTests)
{
	for (int i = 0; i < rateTests.size(); i++) {
		if (rateTests[i].type == eCAL::CMemoryFile::lock_type::mutex){
			if (mutexTests.find(rateTests[i].subCount) != mutexTests.end())
			{
				mutexTests.at(rateTests[i].subCount).push_back(rateTests[i]);
				continue;
			}
			std::vector<rateTest> nextTestList = { rateTests[i] };
			mutexTests.insert({ rateTests[i].subCount, nextTestList });
		}
		else {
			if (rwLockTests.find(rateTests[i].subCount) != rwLockTests.end())
			{
				rwLockTests.at(rateTests[i].subCount).push_back(rateTests[i]);
				continue;
			}
			std::vector<rateTest> nextTestList = { rateTests[i] };
			rwLockTests.insert({ rateTests[i].subCount, nextTestList });
		}
	}
}

int main()
{
	// Get the handle to the current process
	HANDLE hProcess = GetCurrentProcess();

	// Set the process priority to RealTime
	if (!SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS)) {
		std::cerr << "Failed to set process priority to RealTime." << std::endl;
		return 1;
	}

	std::cout << "Process priority set to RealTime." << std::endl << std::endl;
	/*
	std::cout << "mutex" << std::endl << std::endl;
	runLockPerformanceCheck(eCAL::CMemoryFile::lock_type::mutex, 1000000);

	std::cout << "rwLock" << std::endl << std::endl;
	runLockPerformanceCheck(eCAL::CMemoryFile::lock_type::rw_lock, 1000000);

	std::cout << "rwLock 2 subs" << std::endl << std::endl;
	runRwLockPerformance(100000, 2);
	std::cout << "rwLock 5 subs" << std::endl << std::endl;
	runRwLockPerformance(100000, 5);
	std::cout << "rwLock 10 subs" << std::endl << std::endl;
	runRwLockPerformance(100000, 10);
	std::cout << "rwLock 15 subs" << std::endl << std::endl;
	runRwLockPerformance(100000, 15);
	*/
	std::string testResultFileName = "eCAL_base_lock_test";
	int iterations = 250;

	for (int i = 0; i < iterations; i++) {
		std::cout << i << "/" << iterations << std::flush;
		std::cout << "\r";
		
		//run tests with mutex lock
		rwLock = false;
		runTests(testResultFileName, eCAL::CMemoryFile::lock_type::mutex);

		//testResultFileName = "thesis_rw_lock_non_recoverable_test";

		//run tests with rw lock
		rwLock = true;
		runTests(testResultFileName, eCAL::CMemoryFile::lock_type::rw_lock);
	}

	std::map<int, std::vector<rateTest>> mutexTests;
	std::map<int, std::vector<rateTest>> rwLockTests;

	fillTestMap(mutexTests, rwLockTests);
	saveLockData(mutexTests, rwLockTests);

	auto end = std::chrono::system_clock::now().time_since_epoch();

	/*
	std::vector<float> muavgValues;
	std::vector<float> rwavgValues;
	// calculate the total avg for each testcase
	for (int i = 0; i < muTimesList.size(); i++) {
		muavgValues.push_back(std::accumulate(muTimesList[i].begin(), muTimesList[i].end(), 0.0) / float(muTimesList[i].size()));
		rwavgValues.push_back(std::accumulate(rwTimesList[i].begin(), rwTimesList[i].end(), 0.0) / float(rwTimesList[i].size()));
	}

	std::cout << "Mutex:" << std::endl;

	int subNum[] = { 1,3,5,10,20 };
	// print avgs and calulate standard deviations 
	for (int i = 0; i < muavgValues.size(); i++){
		std::cout << "Subs:" << subNum[i] << std::endl;
		std::cout << "avg: " << muavgValues[i] << "microseconds" << std::endl;
		float diviation = 0;
		for (int j = 0; j < muTimesList.size(); j++){
			diviation += (muTimesList[i][j] - muavgValues[i]) * (muTimesList[i][j] - muavgValues[i]);
		}
		std::cout << "standard diviation: " << sqrt(diviation) << std::endl << std::endl ;
	}

	std::cout << "rw-lock" << std::endl;
	for (int i = 0; i < rwavgValues.size(); i++) {
		std::cout << "Subs:" << subNum[i] << std::endl;
		std::cout << "avg: " << rwavgValues[i] << "microseconds" << std::endl;
		float diviation = 0;
		for (int j = 0; j < rwTimesList.size(); j++) {
			diviation += (rwTimesList[i][j] - rwavgValues[i]) * (rwTimesList[i][j] - rwavgValues[i]);
		}
		std::cout << "standard diviation: " << sqrt(diviation) << std::endl << std::endl;
	}
	*/
	if (!SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS)) {
		std::cerr << "Failed to set process priority back to Normal." << std::endl;
		return 1;
	}

return 0;
}

std::vector<TestCaseZeroCopy> createTestCasesZeroCopy()
{
	auto testCases = std::vector<TestCaseZeroCopy>();

	// 1ms calculation time
	testCases.push_back(TestCaseZeroCopy(1, 1, 10, 1));
	testCases.push_back(TestCaseZeroCopy(3, 1, 10, 1));
	testCases.push_back(TestCaseZeroCopy(5, 1, 10, 1));
	testCases.push_back(TestCaseZeroCopy(10, 1, 10, 1));

	// 10ms calculation time
	testCases.push_back(TestCaseZeroCopy(1, 1, 10, 10));
	testCases.push_back(TestCaseZeroCopy(3, 1, 10, 10));
	testCases.push_back(TestCaseZeroCopy(5, 1, 10, 10));
	testCases.push_back(TestCaseZeroCopy(10, 1, 10, 10));

	// 100ms calculation time
	testCases.push_back(TestCaseZeroCopy(1, 1, 10, 100));
	testCases.push_back(TestCaseZeroCopy(3, 1, 10, 100));
	testCases.push_back(TestCaseZeroCopy(5, 1, 10, 100));
	testCases.push_back(TestCaseZeroCopy(10, 1, 10, 100));

	return testCases;
}

std::vector<TestCaseCopy> createTestCasesCopy()
{
	auto testCases = std::vector<TestCaseCopy>();

	// 100 Byte payload
	testCases.push_back(TestCaseCopy(1, 1, 5000, 1));
	testCases.push_back(TestCaseCopy(3, 1, 5000, 1));
	testCases.push_back(TestCaseCopy(5, 1, 5000, 1));
	testCases.push_back(TestCaseCopy(10, 1, 5000, 1));
	testCases.push_back(TestCaseCopy(20, 1, 5000, 1));

	// 1 Kb
	//testCases.push_back(TestCaseCopy(1, 1, 10, 1000));
	//testCases.push_back(TestCaseCopy(3, 1, 10, 1000));
	//testCases.push_back(TestCaseCopy(5, 1, 10, 1000));
	//testCases.push_back(TestCaseCopy(10, 1, 10, 1000));

	// 1 Mb
	//testCases.push_back(TestCaseCopy(1, 1, 10, 1000000));
	//testCases.push_back(TestCaseCopy(3, 1, 10, 1000000));
	//testCases.push_back(TestCaseCopy(5, 1, 10, 1000000));
	//testCases.push_back(TestCaseCopy(10, 1, 10, 1000000));

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
	//std::vector<TestCaseZeroCopy> testCasesZeroCopy = createTestCasesZeroCopy();
	std::vector<TestCaseCopy> testCasesCopy = createTestCasesCopy();

	//run tests
	//runTestsZeroCopy(testCasesZeroCopy, fileName, lock_type);
	runTestsCopy(testCasesCopy, fileName, lock_type);

	// create results protobuf message for test
	//shm::Test_pb message;
	//for (int i = 0; i < testCasesZeroCopy.size(); i++) {
	//	*message.add_zerocopycases() = testCasesZeroCopy[i].getPbTestCaseMessage(false);
	//}
	//for (int i = 0; i < testCasesCopy.size(); i++) {
	//	*message.add_copycases() = testCasesCopy[i].getPbTestCaseMessage(false);
	//}

	// save test results
	//saveTestResults(message, fileName);
	printRateTestMetrics(testCasesCopy);
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
		for (int i = 0; i < testCase.getSubCount(); i++) {
			threadHandles.push_back(createReader(testCase, memoryFile, i));
		}

		// wait for test to finish
		for (int i = 0; i < threadHandles.size(); i++) {
			threadHandles[i].join();
		}

		std::cout << "test completed" << std::endl << std::endl;

		// evaluate test results
		testCase.calculateMetrics();

		// reset writer count for next test case
		writerDoneCount = 0;
	}
}

std::vector<long long> getItrationTimes(std::vector<std::vector<long long>> subTimes, int iteration) {
	std::vector<long long> iterationTimes;
	for (int i = 0; i < subTimes.size(); i++) {
		iterationTimes.push_back(subTimes[i][iteration]);
	}
	return iterationTimes;
}


void printRateTestMetrics(std::vector<TestCaseCopy> testCases)
{
	for (int i = 0; i < testCases.size(); i++) {
		rateTest nextRateTest;
		std::vector<long long> totalTimes;
		std::vector<std::vector<long long>> subStart = testCases[i].getSubBeforeAccess();
		std::vector<std::vector<long long>> subEnd = testCases[i].getSubAfterRelease();
		for (int j = 0; j < testCases[i].getMsgCount(); j++) {
			long long pubTime = testCases[i].getPubAfterRelease()[j] - testCases[i].getPubBeforeAccess()[j];
			std::vector<long long> subStartIterationTimes = getItrationTimes(subStart, j);
			std::vector<long long> subEndIterationTimes = getItrationTimes(subEnd, j);

			long long subTime = *std::max_element(subEndIterationTimes.begin(), subEndIterationTimes.end()) - *std::min_element(subStartIterationTimes.begin(), subStartIterationTimes.end());
			totalTimes.push_back(pubTime + subTime);
		}
		nextRateTest.times = totalTimes;
		long long max = *std::max_element(totalTimes.begin(), totalTimes.end());
		long long min = *std::min_element(totalTimes.begin(), totalTimes.end());
		float avg = std::accumulate(totalTimes.begin(), totalTimes.end(), 0LL) / float(testCases[i].getMsgCount());

		if (rwLock == false) {
			nextRateTest.type = eCAL::CMemoryFile::lock_type::mutex;
		}
		else {
			nextRateTest.type = eCAL::CMemoryFile::lock_type::rw_lock;
		}
		nextRateTest.avgTime = avg;
		nextRateTest.maxTime = max;
		nextRateTest.minTime = min;
		nextRateTest.subCount = testCases[i].getSubCount();
		rateTests.push_back(nextRateTest);
	}
}



void runTestsCopy(std::vector<TestCaseCopy>& testCases, std::string fileName, eCAL::CMemoryFile::lock_type lock_type)
{
	//std::cout << "run test copy" << std::endl << std::endl;

	for (int i = 0; i < testCases.size(); i++) {
		TestCaseCopy& testCase = testCases[i];

		//std::cout << "Test " << i + 1 << " in progress..." << std::endl;

		//needed for reader writer coordination
		totalReaderCount = testCase.getSubCount();
		std::vector<std::thread> workers;

		//create memory file
		eCAL::CMemoryFile memoryFile(lock_type);
		memoryFile.Create("TestCopy", true, testCase.getPayloadSize());

		//add writer as first element
		workers.push_back(createWriter(testCase, memoryFile));

		//add readers
		for (int i = 0; i < testCase.getSubCount(); i++) {
			workers.push_back(createReader(testCase, memoryFile, i));
		}

		//wait for test to finish
		for (int i = 0; i < workers.size(); i++) {
			workers[i].join();
		}
		//std::cout << "test completed" << std::endl << std::endl;
		//testCase.calculateMetrics();
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
	//auto afterAccess = std::chrono::steady_clock::now().time_since_epoch();
	auto afterRelease = std::chrono::steady_clock::now().time_since_epoch();
	for (int i = 0; i < testCase.getMsgCount()+WARMUP; i++) {
		if (contentAvailable == false) {
			//blocks the reader till content is written
			std::unique_lock<std::mutex> w_lock(readerWriterLock);

			beforeAccess = std::chrono::steady_clock::now().time_since_epoch();
			while (!memoryFile.GetWriteAccess(WRITE_ACCESS_TIMEOUT)) {}
			//afterAccess = std::chrono::steady_clock::now().time_since_epoch();

			//memoryFile.WriteBuffer(testCase.getPayload().get()->data(), testCase.getPayloadSize(), 0);
			memoryFile.ReleaseWriteAccess();
			afterRelease = std::chrono::steady_clock::now().time_since_epoch();
			contentAvailable = true;
			writerDoneCount++;
			//allows readers to read written content
			readerWriterSync.notify_all();

			if (i >= WARMUP) {
				//process taken time while readers can read
				testCase.pushToPubBeforeAccessTimes(std::chrono::duration_cast<std::chrono::microseconds>(beforeAccess).count());
				//testCase.pushToPubAfterAccessTimes(std::chrono::duration_cast<std::chrono::milliseconds>(afterAccess).count());
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
	for (int i = 0; i < testCase.getMsgCount(); i++) {
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

		testCase.pushToSubBeforeAccessTimes(std::chrono::duration_cast<std::chrono::milliseconds>(beforeAccess).count(), timesIndex);
		testCase.pushToSubAfterAccessTimes(std::chrono::duration_cast<std::chrono::milliseconds>(afterAccess).count(), timesIndex);
		testCase.pushToSubAfterReleaseTimes(std::chrono::duration_cast<std::chrono::milliseconds>(afterRelease).count(), timesIndex);
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
		beforeAccess = std::chrono::steady_clock::now().time_since_epoch();
		while (!memoryFile.GetReadAccess(READ_ACCESS_TIMEOUT)) {}
		//afterAccess = std::chrono::steady_clock::now().time_since_epoch();

		//memoryFile.Read(_buf.data(), testCase.getPayloadSize(), 0);
		memoryFile.ReleaseReadAccess();

		afterRelease = std::chrono::steady_clock::now().time_since_epoch();

		readerDone();

		if (i >= WARMUP) {
			testCase.pushToSubBeforeAccessTimes(std::chrono::duration_cast<std::chrono::microseconds>(beforeAccess).count(), timesIndex);
			//testCase.pushToSubAfterAccessTimes(std::chrono::duration_cast<std::chrono::milliseconds>(afterAccess).count(), timesIndex);
			testCase.pushToSubAfterReleaseTimes(std::chrono::duration_cast<std::chrono::microseconds>(afterRelease).count(), timesIndex);
		}
	}
}