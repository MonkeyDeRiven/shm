#include "test_case_copy.h"
#include "memory"

TestCaseCopy::TestCaseCopy(int subCount, int pubCount, int messageCount, int payloadSize) : TestCase(subCount, pubCount, messageCount)
{
	this->payload = createPayload(payloadSize);
}

std::shared_ptr<std::string> TestCaseCopy::createPayload(int payloadSize)
{
	return std::make_shared<std::string>(std::string("0", payloadSize));
}