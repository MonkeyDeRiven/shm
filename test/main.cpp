#include <ecal_memfile.h>
#include <ecal_memfile_header.h>

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

const int POINTER_SIZE = 8;

struct TestCaseZeroCopy {
	int subCount;
    int pubCount;
	int msgCount;
    //calculation time in ms
    int calcTime;
	const std::shared_ptr<std::string> payload = std::make_shared<std::string>("0", POINTER_SIZE);
    std::string type = "zerocopy";
};

struct TestCaseCopy {
    int subCount;
    int pubCount;
    int msgCount;
    std::shared_ptr<std::string> payload;
    std::string type = "copy";
};

//Create test cases list
std::vector<TestCaseZeroCopy> createTestCasesZeroCopy();
std::vector<TestCaseCopy> createTestCasesCopy();

//Create test case
TestCaseZeroCopy createTestZeroCopy(int subCount, int pubCount, int msgCount, int calculationTime);
TestCaseCopy createTestCopy(int subCount, int pubCount, int msgCount, int payloadSize);

std::shared_ptr<std::string> createPayload(int payloadSize);

//run tests
void runTests(std::string fileName);
void runTestsZeroCopy(std::string fileName);
void runTestsCopy(std::string fileName);

//reader-writer thread creation
template<typename T>
std::thread createWriter(const T& testCase, eCAL::CMemoryFile& memoryFile);
template<typename T>
std::thread createReader(const T& testCase, eCAL::CMemoryFile& memoryFile, int timesIndex);

//reader-writer tasks
void writerTask(int subCount, int msgCount, const std::shared_ptr<std::string>& payload, eCAL::CMemoryFile& memoryFile);
void readerTaskZeroCopy(int subCount, int msgCount, int timesIndex, int calcTime, const std::shared_ptr<std::string> paylaod, eCAL::CMemoryFile& memoryFile);
void readerTaskCopy(int subCount, int msgCount, int timesIndex, const std::shared_ptr<std::string> payload, eCAL::CMemoryFile& mermoryFile);

//time measurement
void initializeSubTimes(int subCount);
template<typename T>
void saveTestResults(T testCase, std::string fileName);
bool writeTimesToFile(std::string fileName, std::string destPath);
std::vector<unsigned long long> pubTimes;                   //start times for every send msg
std::vector<std::vector<unsigned long long>> subTimes;      //end times for every send msg per subscriber

//thread sync
std::condition_variable readerWriterSync;
std::mutex readerWriterLock;

//reader writer coordination
std::mutex readerDoneLock;
std::atomic<int> readerDoneCount = 0;
std::atomic<int> writerDoneCount = 0;
int totalReaderCount = 0;
bool contentAvailable = false;

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

  runTests(testResultFileName);

  return 0;
}

std::vector<TestCaseZeroCopy> createTestCasesZeroCopy()
{
    auto testCases = std::vector<TestCaseZeroCopy>();
    
    //one sub
    testCases.push_back(createTestZeroCopy(1, 1, 10, 1));
    testCases.push_back(createTestZeroCopy(1, 1, 10, 10));
    testCases.push_back(createTestZeroCopy(1, 1, 10, 100));
    testCases.push_back(createTestZeroCopy(1, 1, 10, 1000));

    //three subs
    testCases.push_back(createTestZeroCopy(3, 1, 10, 1));
    testCases.push_back(createTestZeroCopy(3, 1, 10, 10));
    testCases.push_back(createTestZeroCopy(3, 1, 10, 100));
    testCases.push_back(createTestZeroCopy(3, 1, 10, 1000));

    //five subs
    testCases.push_back(createTestZeroCopy(5, 1, 10, 1));
    testCases.push_back(createTestZeroCopy(5, 1, 10, 10));
    testCases.push_back(createTestZeroCopy(5, 1, 10, 100));
    testCases.push_back(createTestZeroCopy(5, 1, 10, 1000));

    //ten subs
    testCases.push_back(createTestZeroCopy(10, 1, 10, 1));
    testCases.push_back(createTestZeroCopy(10, 1, 10, 10));
    testCases.push_back(createTestZeroCopy(10, 1, 10, 100));
    testCases.push_back(createTestZeroCopy(10, 1, 10, 1000));

    return testCases;
}

std::vector<TestCaseCopy> createTestCasesCopy() 
{
    auto testCases = std::vector<TestCaseCopy>();
    //one sub
    testCases.push_back(createTestCopy(1, 1, 10, 1));
    testCases.push_back(createTestCopy(1, 1, 10, 1000));
    testCases.push_back(createTestCopy(1, 1, 10, 1000000));
    testCases.push_back(createTestCopy(1, 1, 10, 1000000000));

    //three subs
    testCases.push_back(createTestCopy(3, 1, 10, 1));
    testCases.push_back(createTestCopy(3, 1, 10, 1000));
    testCases.push_back(createTestCopy(3, 1, 10, 1000000));
    testCases.push_back(createTestCopy(3, 1, 10, 1000000000));

    //five subs
    //testCases.push_back(createTestCopy(5, 1, 10, 1));
    //testCases.push_back(createTestCopy(5, 1, 10, 1000));
    //testCases.push_back(createTestCopy(5, 1, 10, 1000000));
    //testCases.push_back(createTestCopy(5, 1, 10, 1000000000));

    //ten subs
    //testCases.push_back(createTestCopy(10, 1, 10, 1));
    //testCases.push_back(createTestCopy(10, 1, 10, 1000));
    //testCases.push_back(createTestCopy(10, 1, 10, 1000000));
    //testCases.push_back(createTestCopy(10, 1, 10, 1000000000));
    

    return testCases;
}

TestCaseZeroCopy createTestZeroCopy(int subCount, int pubCount, int msgCount, int calculationTime)
{
    TestCaseZeroCopy testCase;
    testCase.subCount = subCount;
    testCase.pubCount = pubCount;
    testCase.msgCount = msgCount;
    testCase.calcTime = calculationTime;

    return testCase;
}

TestCaseCopy createTestCopy(int subCount, int pubCount, int msgCount, int payloadSize)
{
    TestCaseCopy testCase;
    testCase.subCount = subCount;
    testCase.pubCount = pubCount;
    testCase.msgCount = msgCount;
    testCase.payload = std::move(createPayload(payloadSize));

    return testCase;

}

std::shared_ptr<std::string> createPayload(int payloadSize)
{
    return std::make_shared<std::string>(payloadSize, '0');
}

void initializeSubTimes(int subCount) 
{
    for (int i = 0; i < subCount; i++) {
        subTimes.push_back(std::vector<unsigned long long>());
    }
}

template<typename T>
void saveTestResults(T testCase, std::string fileName)
{
    std::cout << "write results..." << std::endl;

    std::ofstream file;
    file.open(fileName, std::ios::app);
    if (file.is_open()) {
        try{
            file << testCase.subCount << std::endl;
            file << testCase.msgCount << std::endl;
            if constexpr (std::is_convertible_v<T, TestCaseZeroCopy>) {
                file << testCase.calcTime << std::endl;
            }
            else {
                file << testCase.payload->size() << std::endl;
            }
            file << "pub" << std::endl;
            //first publisher time is the start time of the measurement, so we write a 0
            file << "0" << std::endl;
            for (long long i = 1; i < pubTimes.size(); i++) {
                file << pubTimes[i] - pubTimes[i-1] << std::endl;
            }
            for (int i = 0; i < subTimes.size(); i++) {
                file << "sub" << std::endl;
                for (int j = 0; j < subTimes[i].size(); j++) {
                    long long time = subTimes[i][j] - pubTimes[j];
                    file << time << std::endl;
                }
            }
            file << "end" << std::endl;
            file.close();
            std::cout << "results written!" << std::endl << std::endl;
        }
        catch (std::exception e)
        {
            std::cout << "could not save results due to: " << e.what() << std::endl << std::endl;
        }
    }
    else {
        std::cout << "could not open file, abort all tests!";
        exit(1);
    }

    //get rid of test timings
    subTimes.clear();
    pubTimes.clear();
}

void runTests(std::string fileName) {
    runTestsZeroCopy(fileName);
    runTestsCopy(fileName);
}

void runTestsZeroCopy(std::string fileName)
{
    std::vector<TestCaseZeroCopy> testCases = createTestCasesZeroCopy();

    std::ofstream file;
    file.open(fileName);

    if (file.is_open()) {
        file << testCases[0].type << std::endl;
    }

    file.close();

    std::cout << "run zero copy tests" << std::endl << std::endl;

    for (int i = 0; i < testCases.size(); i++) {
        file.open(fileName, std::ios::app);
        if (file.is_open()) {
            file << "test " << i << std::endl;
        }

        TestCaseZeroCopy testCase = testCases[i];

        initializeSubTimes(testCase.subCount);

        std::cout << "test " << i + 1 << " in progress..." << std::endl;
        //Create memoryFile
        eCAL::CMemoryFile memoryFile;
        //reserve 50 more bytes in case a header is written
        memoryFile.Create("TestZeroCopy", true, sizeof(testCase.payload.get()) + 50);

        //needed for reader writer coordination
        totalReaderCount = testCase.subCount;
        std::vector<std::thread> workers;

        //add writer as first element
        workers.push_back(createWriter(testCase, memoryFile));

        //add reader
        for (int i = 0; i < testCase.subCount; i++) {
            workers.push_back(createReader(testCase, memoryFile, i));
        }

        //wait for test to finish
        for (int i = 0; i < workers.size(); i++) {
            workers[i].join();
        }

        std::cout << "test completed" << std::endl;
        saveTestResults(testCase, fileName);
        writerDoneCount = 0;
    }
}

void runTestsCopy(std::string fileName)
{
    std::vector<TestCaseCopy> testCases = createTestCasesCopy();

    std::ofstream file;
    file.open(fileName, std::ios::app);

    if (file.is_open()) {
        file << testCases[0].type << std::endl;
    }

    file.close();

    std::cout << "run test copy" << std::endl << std::endl;

    for (int i = 0; i < testCases.size(); i++) {
        TestCaseCopy testCase = testCases[i];
        std::cout << "Test " << i+1 << " in progress..." << std::endl;
        //needed for reader writer coordination
        totalReaderCount = testCase.subCount;
        std::vector<std::thread> workers;

        //create memory file
        eCAL::CMemoryFile memoryFile;
        memoryFile.Create("TestCopy", true, testCase.payload->size() + 50);

        //add writer as first element
        workers.push_back(createWriter(testCase, memoryFile));

        //add readers
        for (int i = 0; i < testCase.subCount; i++) {
            workers.push_back(createReader(testCase, memoryFile, i));
        }

        //wait for test to finish
        for (int i = 0; i < workers.size(); i++) {
            workers[i].join();
        }
        std::cout << "test completed" << std::endl;
        saveTestResults(testCase, fileName);
        writerDoneCount = 0;
    }
}

template<typename T>
std::thread createWriter(const T& testCase, eCAL::CMemoryFile& memoryFile) {
    return std::thread([&]() {
        writerTask(testCase.subCount, testCase.msgCount, testCase.payload, memoryFile);
        });
}

template<typename T>
std::thread createReader(const T& testCase, eCAL::CMemoryFile& memoryFile, int timesIndex)
{
    if constexpr (std::is_convertible_v<T, TestCaseZeroCopy>) {
        return std::thread([&testCase, &memoryFile, timesIndex]() {
            readerTaskZeroCopy(testCase.subCount, testCase.msgCount, timesIndex, testCase.calcTime, testCase.payload, memoryFile);
            });
    }
    return std::thread([&testCase, &memoryFile, timesIndex]() {
        readerTaskCopy(testCase.subCount, testCase.msgCount, timesIndex, testCase.payload, memoryFile);
        });
}

void writerTask(int subCount, int msgCount, const std::shared_ptr<std::string>& payload, eCAL::CMemoryFile& memoryFile)
{
   auto start = std::chrono::steady_clock::now().time_since_epoch();
    for (int i = 0; i < msgCount; i++) {
        if (contentAvailable == false) {
            //blocks the reader till content is written
            std::unique_lock<std::mutex> w_lock(readerWriterLock);

            while (!memoryFile.GetWriteAccess(WRITE_ACCESS_TIMEOUT)) {}
            start = std::chrono::steady_clock::now().time_since_epoch();
            
            memoryFile.WriteBuffer(payload->data(), payload->size(), 0);
            memoryFile.ReleaseWriteAccess();
            contentAvailable = true;
            writerDoneCount++;

            //allows readers to read written content
            readerWriterSync.notify_all();

            //process taken time while readers can read
            long long time = std::chrono::duration_cast<std::chrono::milliseconds>(start).count();
            pubTimes.push_back(time);
            readerWriterSync.wait(w_lock, [] { return !contentAvailable; });
        }
    }
}

void readerTaskZeroCopy(int subCount, int msgCount, int timesIndex, int calcTime, const std::shared_ptr<std::string> payload, eCAL::CMemoryFile& memoryFile)
{
    std::vector<std::string> _buf(POINTER_SIZE);
    auto end = std::chrono::steady_clock::now().time_since_epoch();
    for (int i = 0; i < msgCount; i++) {
        //wait till content is available
        std::unique_lock<std::mutex> r_lock(readerWriterLock);
        readerWriterSync.wait(r_lock, [=] { return writerDoneCount == i+1; });
        //r_lock.unlock();

        //aquire read access, could already be aquired by different reader
        while (!memoryFile.GetReadAccess(READ_ACCESS_TIMEOUT)) {}
        end = std::chrono::steady_clock::now().time_since_epoch();
        
        memoryFile.Read(_buf.data(), payload->size(), 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(calcTime));
        
        memoryFile.ReleaseReadAccess();
        readerDone();

        long long time = std::chrono::duration_cast<std::chrono::milliseconds>(end).count();
        subTimes[timesIndex].push_back(time);
    }
}

void readerTaskCopy(int subCount, int msgCount, int timesIndex, const std::shared_ptr<std::string> payload, eCAL::CMemoryFile& memoryFile) 
{
    std::vector<char> _buf = std::vector<char>(payload->size());
    auto end = std::chrono::steady_clock::now().time_since_epoch();
    for (int i = 0; i < msgCount; i++) {
        //wait till content is available
        std::unique_lock<std::mutex> r_lock(readerWriterLock);
        readerWriterSync.wait(r_lock, [=]() { return writerDoneCount == i+1; });

        //unlock so other readers are not blocked
        r_lock.unlock();

        //aquire read access, could already be aquired by different reader
        while (!memoryFile.GetReadAccess(READ_ACCESS_TIMEOUT)){}

        memoryFile.Read(_buf.data(), payload->size(), 0);
        memoryFile.ReleaseReadAccess();

        //time it took for the message to get recieved
        end = std::chrono::steady_clock::now().time_since_epoch();

        readerDone();

        long long time = std::chrono::duration_cast<std::chrono::milliseconds>(end).count();
        subTimes[timesIndex].push_back(time);
    }
}