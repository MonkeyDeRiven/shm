#pragma once
#include "test_case.h"
#include "test_case.pb.h"
#include <memory>
#include <string> 

const int POINTER_SIZE = 8;

class TestCaseZeroCopy : public TestCase{
public:
	TestCaseZeroCopy(int subCount, int pubCount, int messageCount, int calculationTime);

	int getCalculationTime();

	std::shared_ptr<std::string> createPayload(int payloadSize) override;

	shm::TestCaseZeroCopy_pb getPbTestCaseMessage(bool withRawData);

private:
	int calculationTime;
};