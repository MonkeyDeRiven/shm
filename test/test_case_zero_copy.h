#pragma once
#include "test_case.h"
#include <memory>
#include <string> 

const int POINTER_SIZE = 8;

class TestCaseZeroCopy : public TestCase{
public:
	TestCaseZeroCopy(int subCount, int pubCount, int messageCount, int calculationTime);

	int getCalculationTime();

	std::shared_ptr<std::string> createPayload(int payloadSize) override;
private:
	int calculationTime;
};