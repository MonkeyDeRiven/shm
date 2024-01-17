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
	return std::make_shared<std::string>(payloadSize, '0');
}

shm::TestCaseZeroCopy_pb TestCaseZeroCopy::getPbTestCaseMessage(bool rawData)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	shm::TestCaseZeroCopy_pb message;

	auto header = message.mutable_header();

	header->set_subcount(this->subCount);
	header->set_pubcount(this->pubCount);
	header->set_msgcount(this->msgCount);
	header->set_calculationtime(this->calculationTime);

	auto metrics = message.mutable_metrics();

	metrics->set_totalduration(this->totalDuration);
	metrics->set_totallocktime(this->totalLockTime);
	metrics->set_totalsubcriberlocktime(this->totalSubcriberLockTime);

	metrics->set_avgiterationduration(this->avgIterationDuration);
	metrics->set_maxiterationduration(this->maxIterationDuration);
	metrics->set_miniterationduration(this->minIterationDuration);

	metrics->set_avgiterationlocktime(this->avgIterationLockTime);
	metrics->set_maxiterationlocktime(this->maxIterationLockTime);
	metrics->set_miniterationlocktime(this->minIterationLockTime);

	metrics->set_avgsubscriberiterationlocktime(this->avgSubscriberIterationLockTime);
	metrics->set_maxsubscriberiterationlocktime(this->maxSubscriberIterationLockTime);
	metrics->set_minsubscriberiterationlocktime(this->minSubscriberIterationLockTime);

	metrics->set_avgpublatency(this->avgPubLatency);
	metrics->set_maxpublatency(this->maxPubLatency);
	metrics->set_minpublatency(this->minPubLatency);

	for (int i = 0; i < this->subLatencies.size(); i++) {
		metrics->add_avgsublatencies(this->avgSubLatencies[i]);
	}

	for (int i = 0; i < this->maxSubLatencies.size(); i++) {
		metrics->add_maxsublatencies(this->maxSubLatencies[i]);
	}

	for (int i = 0; i < minSubLatencies.size(); i++) {
		metrics->add_minsublatencies(this->minSubLatencies[i]);
	}

	if (rawData) {
		auto raw = message.mutable_raw();

		for (int i = 0; i < this->pubBeforeAccessTimes.size(); i++) {
			raw->add_pubbeforeaccesstimes(this->pubBeforeAccessTimes[i]);
		}

		for (int i = 0; i < this->pubAfterAccessTimes.size(); i++) {
			raw->add_pubafteraccesstimes(this->pubAfterAccessTimes[i]);
		}

		for (int i = 0; i < this->pubAfterReleaseTimes.size(); i++) {
			raw->add_pubafterreleasetimes(this->pubAfterReleaseTimes[i]);
		}

		for (int i = 0; i < this->subBeforeAccessTimes.size(); i++) {
			auto subTimes = raw->add_subbeforeaccesstimes();
			for (int j = 0; j < this->subBeforeAccessTimes[i].size(); j++) {
				subTimes->add_times(this->subBeforeAccessTimes[i][j]);
			}
		}

		for (int i = 0; i < this->subAfterAccessTimes.size(); i++) {
			auto subTimes = raw->add_subafteraccesstimes();
			for (int j = 0; j < this->subAfterAccessTimes[i].size(); j++) {
				subTimes->add_times(this->subAfterAccessTimes[i][j]);
			}
		}

		for (int i = 0; i < this->subAfterReleaseTimes.size(); i++) {
			auto subTimes = raw->add_subafterreleasetimes();
			for (int j = 0; j < this->subAfterReleaseTimes[i].size(); j++) {
				subTimes->add_times(this->subAfterReleaseTimes[i][j]);
			}
		}
	}
	return message;
}

