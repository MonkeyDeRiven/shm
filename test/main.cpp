#include <ecal_memfile.h>
#include <ecal_memfile_header.h>

#include "test_case.h"
#include "test_case_copy.h"
#include "test_case_zero_copy.h"

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

const bool SEND_RAW_DATA = false;

//Create test cases list
std::vector<TestCaseZeroCopy> createTestCasesZeroCopy();
std::vector<TestCaseCopy> createTestCasesCopy();

//run tests
void runTests(std::string fileName);
void runTestsZeroCopy(std::string fileName);
void runTestsCopy(std::string fileName);

//reader-writer thread creation
template<typename T>
std::thread createWriter(T& testCase, eCAL::CMemoryFile& memoryFile);

std::thread createReader(TestCase&, eCAL::CMemoryFile& memoryFile, int timesIndex);

//reader-writer tasks
void writerTask(TestCase& testCase, eCAL::CMemoryFile& memoryFile);
void readerTaskZeroCopy(TestCaseZeroCopy& testCase, eCAL::CMemoryFile& memoryFile, int timesIndex);
void readerTaskCopy(TestCaseCopy& testCase, eCAL::CMemoryFile& mermoryFile, int timesIndex);

//time measurement
void saveTestResults(TestCase& testCase, std::string fileName);

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
    testCases.push_back(TestCaseZeroCopy(1, 1, 10, 1));
    testCases.push_back(TestCaseZeroCopy(1, 1, 10, 10));
    testCases.push_back(TestCaseZeroCopy(1, 1, 10, 100));
    testCases.push_back(TestCaseZeroCopy(1, 1, 10, 1000));

    //three subs
    //testCases.push_back(TestCaseZeroCopy(3, 1, 10, 1));
    //testCases.push_back(TestCaseZeroCopy(3, 1, 10, 10));
    //testCases.push_back(TestCaseZeroCopy(3, 1, 10, 100));
    //testCases.push_back(TestCaseZeroCopy(3, 1, 10, 1000));

    //five subs
    //testCases.push_back(TestCaseZeroCopy(5, 1, 10, 1));
    //testCases.push_back(TestCaseZeroCopy(5, 1, 10, 10));
    //testCases.push_back(TestCaseZeroCopy(5, 1, 10, 100));
    //testCases.push_back(TestCaseZeroCopy(5, 1, 10, 1000));

    //ten subs
    //testCases.push_back(TestCaseZeroCopy(10, 1, 10, 1));
    //testCases.push_back(TestCaseZeroCopy(10, 1, 10, 10));
    //testCases.push_back(TestCaseZeroCopy(10, 1, 10, 100));
    //testCases.push_back(TestCaseZeroCopy(10, 1, 10, 1000));

    return testCases;
}

std::vector<TestCaseCopy> createTestCasesCopy() 
{
    auto testCases = std::vector<TestCaseCopy>();
    //one sub
    testCases.push_back(TestCaseCopy(1, 1, 10, 1));
    testCases.push_back(TestCaseCopy(1, 1, 10, 1000));
    testCases.push_back(TestCaseCopy(1, 1, 10, 1000000));
    testCases.push_back(TestCaseCopy(1, 1, 10, 1000000000));

    //three subs
    //testCases.push_back(TestCaseCopy(3, 1, 10, 1));
    //testCases.push_back(TestCaseCopy(3, 1, 10, 1000));
    //testCases.push_back(TestCaseCopy(3, 1, 10, 1000000));
    //testCases.push_back(TestCaseCopy(3, 1, 10, 1000000000));

    //five subs
    //testCases.push_back(TestCaseCopy(5, 1, 10, 1));
    //testCases.push_back(TestCaseCopy(5, 1, 10, 1000));
    //testCases.push_back(TestCaseCopy(5, 1, 10, 1000000));
    //testCases.push_back(TestCaseCopy(5, 1, 10, 1000000000));

    //ten subs
    //testCases.push_back(TestCaseCopy(10, 1, 10, 1));
    //testCases.push_back(TestCaseCopy(10, 1, 10, 1000));
    //testCases.push_back(TestCaseCopy(10, 1, 10, 1000000));
    //testCases.push_back(TestCaseCopy(10, 1, 10, 1000000000));
    

    return testCases;
}



void saveTestResults(TestCase& testCase, std::string fileName)
{
    std::ofstream file;
    file.open(fileName, std::ios::app);

    std::cout << "save results..." << std::endl;

    if(file.is_open()){
        std::string data;
        if (dynamic_cast<TestCaseCopy*>(&testCase) != nullptr) {
            data = dynamic_cast<TestCaseCopy&>(testCase).getPbTestCaseMessage(SEND_RAW_DATA).SerializeAsString();
        }
        else {
            data = dynamic_cast<TestCaseZeroCopy&>(testCase).getPbTestCaseMessage(SEND_RAW_DATA).SerializeAsString();
        }
        file << data.size() << "\n";
        file << data;
        file.close();
        std::cout << "results saved!" << std::endl << std::endl;
    }
    else {
        std::cout << "ERROR: could not save test results!" << std::endl << std::endl;
    }
}

void runTests(std::string fileName) {
    //clear file content
    std::ofstream file;
    file.open(fileName, std::ofstream::trunc);
    file.close();

    runTestsZeroCopy(fileName);
    runTestsCopy(fileName);
}

void runTestsZeroCopy(std::string fileName)
{
    std::vector<TestCaseZeroCopy> testCases = createTestCasesZeroCopy();

    std::ofstream file;

    file.open(fileName, std::ios::app);
    
    if (file.is_open()) {
        file << "zerocopy\n";
        file.close();
    }
    else {
        std::cout << "could not open file, please try to resolve this issue!";
        return;
    }


    std::cout << "run zero copy tests" << std::endl << std::endl;

    for (int i = 0; i < testCases.size(); i++) {

        TestCaseZeroCopy testCase = testCases[i];

        std::cout << "test " << i + 1 << " in progress..." << std::endl;
        //Create memoryFile
        eCAL::CMemoryFile memoryFile;
        //reserve 50 more bytes in case a header is written
        memoryFile.Create("TestZeroCopy", true, testCase.getPayloadSize());

        //needed for reader writer coordination
        totalReaderCount = testCase.getSubCount();
        std::vector<std::thread> workers;

        //add writer as first element
        workers.push_back(createWriter(testCase, memoryFile));

        //add reader
        for (int i = 0; i < testCase.getSubCount(); i++) {
            workers.push_back(createReader(testCase, memoryFile, i));
        }

        //wait for test to finish
        for (int i = 0; i < workers.size(); i++) {
            workers[i].join();
        }

        std::cout << "test completed" << std::endl;
        testCase.calculateMetrics();
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
        file << "copy\n";
        file.close();
    }
    else {
        std::cout << "could not open file, please try to resolve this issue!";
        return;
    }

    std::cout << "run test copy" << std::endl << std::endl;

    for (int i = 0; i < testCases.size(); i++) {
        TestCaseCopy testCase = testCases[i];

        std::cout << "Test " << i+1 << " in progress..." << std::endl;

        //needed for reader writer coordination
        totalReaderCount = testCase.getSubCount();
        std::vector<std::thread> workers;

        //create memory file
        eCAL::CMemoryFile memoryFile;
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
        std::cout << "test completed" << std::endl;
        testCase.calculateMetrics();
        saveTestResults(testCase, fileName);
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
    for (int i = 0; i < testCase.getMsgCount(); i++) {
        if (contentAvailable == false) {
            //blocks the reader till content is written
            std::unique_lock<std::mutex> w_lock(readerWriterLock);

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
            testCase.pushToPubBeforeAccessTimes(std::chrono::duration_cast<std::chrono::milliseconds>(beforeAccess).count());
            testCase.pushToPubAfterAccessTimes(std::chrono::duration_cast<std::chrono::milliseconds>(afterAccess).count());
            testCase.pushToPubAfterReleaseTimes(std::chrono::duration_cast<std::chrono::milliseconds>(afterRelease).count());
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
        readerWriterSync.wait(r_lock, [=] { return writerDoneCount == i+1; });
        //r_lock.unlock();

        //aquire read access, could already be aquired by different reader
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
    for (int i = 0; i < testCase.getMsgCount(); i++) {
        //wait till content is available
        std::unique_lock<std::mutex> r_lock(readerWriterLock);
        readerWriterSync.wait(r_lock, [=]() { return writerDoneCount == i+1; });

        //unlock so other readers are not blocked
        r_lock.unlock();

        //aquire read access, could already be aquired by different reader
        while (!memoryFile.GetReadAccess(READ_ACCESS_TIMEOUT)){}
        afterAccess = std::chrono::steady_clock::now().time_since_epoch();

        memoryFile.Read(_buf.data(), testCase.getPayloadSize(), 0);
        memoryFile.ReleaseReadAccess();

        afterRelease = std::chrono::steady_clock::now().time_since_epoch();

        readerDone();

        testCase.pushToSubBeforeAccessTimes(std::chrono::duration_cast<std::chrono::milliseconds>(beforeAccess).count(), timesIndex);
        testCase.pushToSubAfterAccessTimes(std::chrono::duration_cast<std::chrono::milliseconds>(afterAccess).count(), timesIndex);
        testCase.pushToSubAfterReleaseTimes(std::chrono::duration_cast<std::chrono::milliseconds>(afterRelease).count(), timesIndex);
    }
}