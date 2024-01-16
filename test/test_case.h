#pragma once
#include <vector>
#include <string>
#include <memory>

class TestCase {
public:
	TestCase(int subCount, int pubCount, int messageCount);

	virtual std::shared_ptr<std::string> createPayload(int payloadSize) = 0;

	void pushToPubAfterAccessTimes(long long time);
	void pushToPubAfterReleaseTimes(long long time);
	void pushToSubAfterAccessTimes(long long time, int index);
	void pushToSubAfterReleaseTimes(long long time, int index);

	int getSubCount();
	int getPubCount();
	int getMsgCount();
	int getPayloadSize();
	std::shared_ptr<std::string> getPayload();

	//metric methods
	void calculateMetrics();

	std::vector<long long> getSubscriberLockTimes();
	std::vector<long long> getIterationLockTimes();
	std::vector<long long> getIterationDurations();

	long long getTotalDuration();
	long long getTotalLockTime();
	long long getTotalSubscriberLockTime();

	//metrics helper methods
	long long getMaxSubAfterReleaseTimeForIteration(int iteration);
	long long getMinSubAfterAccessTimeForIteration(int iteration);

	long long getTime(int i) {
		return this->pubAfterAccessTimes[i];
	}

protected:
	int subCount;
	int pubCount;
	int msgCount;
	std::shared_ptr<std::string> payload;

	std::vector<long long> pubAfterAccessTimes;
	std::vector<long long> pubAfterReleaseTimes;
	std::vector<std::vector<long long>> subAfterAccessTimes;
	std::vector<std::vector<long long>> subAfterReleaseTimes;

private:

	//metrics
	long long totalDuration;
	long long totalLockTime;
	long long totalSubcriberLockTime;

	//lock time metrics
	std::vector<long long> iterationLockTimes;
	float avgIterationLockTime;
	long long maxIterationLockTime;
	long long minIterationLockTime;

	//subscriber lock time metrics
	std::vector<long long> subscriberLockTimes;
	float avgSubscriberIterationLockTime;
	long long maxSubscriberIterationLockTime;
	long long minSubscriberIterationLockTime;

	//test case duration metrics
	std::vector<long long> iterationDurations;
	float avgIterationDuration;
	long long maxIterationDuration;
	long long minIterationDuration;

	void initializeSubTimes();
};