#pragma once
#include "test_case.h"
#include "test_case.pb.h"

class TestCaseCopy : public TestCase {
public:
	TestCaseCopy(int subCount, int pubCount, int messageCount, int payloadSize);

	std::shared_ptr<std::string> createPayload(int payloadSize) override;
	shm::TestCaseCopy_pb getPbTestCaseMessage(bool withRawData);

private:

};