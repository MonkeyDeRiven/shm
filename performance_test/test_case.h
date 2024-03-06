#pragma once
#include <vector>
#include <string>
#include <memory>
#include "test_case.pb.h"

class TestCase {
public:
	TestCase() {};
	TestCase(int subCount, int pubCount, int messageCount);

	virtual std::shared_ptr<std::string> createPayload(int payloadSize);

	void pushToPubBeforeAccessTimes(long long);
	void pushToPubAfterAccessTimes(long long time);
	void pushToPubAfterReleaseTimes(long long time);
	void pushToSubBeforeAccessTimes(long long time, int index);
	void pushToSubAfterAccessTimes(long long time, int index);
	void pushToSubAfterReleaseTimes(long long time, int index);

	int getSubCount();
	int getPubCount();
	int getMsgCount();
	int getPayloadSize();
	std::shared_ptr<std::string> getPayload();

	//Transforms the captured times, so that PubBeforeAccessTime[0] is the begin of the measurement and equals 0
	void adjustTimeFrame();

	//metric methods
	void calculateMetrics();

	//get times workaround
	std::vector<long long> getPubBeforeAccess() { return this->pubBeforeAccessTimes; };
	std::vector<long long> getPubAfterRelease() { return this->pubAfterReleaseTimes; };
	std::vector<std::vector<long long>> getSubBeforeAccess() { return this->subBeforeAccessTimes; };
	std::vector<std::vector<long long>> getSubAfterRelease() { return this->subAfterReleaseTimes; };


protected:
	int subCount;
	int pubCount;
	int msgCount;
	std::shared_ptr<std::string> payload;

	std::vector<long long> pubBeforeAccessTimes;
	std::vector<long long> pubAfterAccessTimes;
	std::vector<long long> pubAfterReleaseTimes;
	std::vector<std::vector<long long>> subBeforeAccessTimes;
	std::vector<std::vector<long long>> subAfterAccessTimes;
	std::vector<std::vector<long long>> subAfterReleaseTimes;

	// metrics
	long long totalDuration;
	long long totalLockTime;
	long long totalSubcriberLockTime;

	// lock time metrics
	std::vector<long long> iterationLockTimes;
	float avgIterationLockTime;
	long long maxIterationLockTime;
	long long minIterationLockTime;

	// subscriber lock time metrics
	std::vector<long long> subscriberLockTimes;
	float avgSubscriberIterationLockTime;
	long long maxSubscriberIterationLockTime;
	long long minSubscriberIterationLockTime;

	// test case duration metrics
	std::vector<long long> iterationDurations;
	float avgIterationDuration;
	long long maxIterationDuration;
	long long minIterationDuration;

	// latency
	std::vector<long long> pubLatencies;
	float avgPubLatency;
	long long maxPubLatency;
	long long minPubLatency;

	std::vector<std::vector<long long>> subLatencies;
	std::vector<float> avgSubLatencies;
	std::vector<long long> maxSubLatencies;
	std::vector<long long> minSubLatencies;

private:

	void initializeSubTimes();
};