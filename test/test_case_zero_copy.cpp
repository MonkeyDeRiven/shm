#include "test_case_zero_copy.h"

TestCaseZeroCopy::TestCaseZeroCopy(int subCount, int pubCount, int msgCount, int calculationTime) : TestCase(subCount, pubCount, msgCount)
{
	this->calculationTime = calculationTime;
	this->payload = createPayload(POINTER_SIZE);
}

int TestCaseZeroCopy::getCalculationTime() 
{
	return calculationTime;
}

std::shared_ptr<std::string> TestCaseZeroCopy::createPayload(int payloadSize) 
{
	return std::make_shared<std::string>(std::string("0", payloadSize));
}