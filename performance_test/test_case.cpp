#include "test_case.h"
#include "metric_calculator.h"
#include <algorithm>
#include <numeric>


//helper functions to adjust time frame
void correctSubTimes(std::vector<std::vector<long long>>& times, long long startTime);
void correctPubTimes(std::vector<long long>& times, long long startTime);

TestCase::TestCase(int subCount, int pubCount, int msgCount)
{
	this->subCount = subCount;
	this->pubCount = pubCount;
	this->msgCount = msgCount;

	initializeSubTimes();
}

void TestCase::initializeSubTimes()
{
	for (int i = 0; i < this->subCount; i++) {
		this->subBeforeAccessTimes.push_back(std::vector<long long>());
		this->subAfterAccessTimes.push_back(std::vector<long long>());
		this->subAfterReleaseTimes.push_back(std::vector<long long>());
		this->subLatencies.push_back(std::vector<long long>());
	}
}

std::shared_ptr<std::string> TestCase::createPayload(int payloadSize)
{
	return std::make_shared<std::string>("");
}

void TestCase::pushToPubBeforeAccessTimes(long long time)
{
	this->pubBeforeAccessTimes.push_back(time);
}

void TestCase::pushToPubAfterAccessTimes(long long time)
{
	this->pubAfterAccessTimes.push_back(time);
}

void TestCase::pushToPubAfterReleaseTimes(long long time)
{
	this->pubAfterReleaseTimes.push_back(time);
}

void TestCase::pushToSubBeforeAccessTimes(long long time, int index)
{
	this->subBeforeAccessTimes[index].push_back(time);
}

void TestCase::pushToSubAfterAccessTimes(long long time, int index)
{
	this->subAfterAccessTimes[index].push_back(time);
}

void TestCase::pushToSubAfterReleaseTimes(long long time, int index)
{
	this->subAfterReleaseTimes[index].push_back(time);
}

int TestCase::getSubCount()
{
	return this->subCount;
}

int TestCase::getPubCount()
{
	return this->pubCount;
}

int TestCase::getMsgCount()
{
	return this->msgCount;
}

int TestCase::getPayloadSize() {
	return this->payload.get()->size();
}

std::shared_ptr<std::string> TestCase::getPayload() {
	return this->payload;
}

void TestCase::adjustTimeFrame()
{
	long long startTime = this->pubBeforeAccessTimes[0];
	this->pubBeforeAccessTimes[0] = 0;
	for (int i = 1; i < this->pubBeforeAccessTimes.size(); i++) {
		this->pubBeforeAccessTimes[i] = this->pubBeforeAccessTimes[i] - startTime;
	}
	correctPubTimes(this->pubAfterAccessTimes, startTime);
	correctPubTimes(this->pubAfterReleaseTimes, startTime);
	correctSubTimes(this->subBeforeAccessTimes, startTime);
	correctSubTimes(this->subAfterAccessTimes, startTime);
	correctSubTimes(this->subAfterReleaseTimes, startTime);
}


void correctPubTimes(std::vector<long long>& times, long long startTime) {
	for (int i = 0; i < times.size(); i++) {
		times[i] = times[i] - startTime;
	}
}

void correctSubTimes(std::vector<std::vector<long long>>& times, long long startTime) {
	for (int i = 0; i < times.size(); i++) {
		for (int j = 0; j < times[i].size(); j++) {
			times[i][j] = times[i][j] - startTime;
		}
	}
}

//metric methods

void TestCase::calculateMetrics()
{
	adjustTimeFrame();

	MetricCalculator calculator;

	this->subscriberLockTimes = calculator.getSubscriberLockTimes(subAfterAccessTimes, subAfterReleaseTimes);
	this->iterationLockTimes = calculator.getIterationLockTimes(subscriberLockTimes, pubAfterAccessTimes, pubAfterReleaseTimes);
	this->iterationDurations = calculator.getIterationDurations(pubAfterAccessTimes, subAfterReleaseTimes);

	this->avgIterationLockTime = calculator.getAvgTime(iterationLockTimes);
	this->maxIterationLockTime = calculator.getMaxTime(iterationLockTimes);
	this->minIterationLockTime = calculator.getMinTime(iterationLockTimes);

	this->avgSubscriberIterationLockTime = calculator.getAvgTime(subscriberLockTimes); 
	this->maxSubscriberIterationLockTime = calculator.getMaxTime(subscriberLockTimes); 
	this->minSubscriberIterationLockTime = calculator.getMinTime(subscriberLockTimes); 

	this->avgIterationDuration = calculator.getAvgTime(iterationDurations); 
	this->maxIterationDuration = calculator.getMaxTime(iterationDurations);
	this->minIterationDuration = calculator.getMaxTime(iterationDurations);

	this->totalDuration = calculator.getTotalDuration(subAfterReleaseTimes);
	this->totalLockTime = calculator.getTotalLockTime(subscriberLockTimes, pubAfterAccessTimes, pubAfterReleaseTimes);
	this->totalSubcriberLockTime = calculator.getTotalSubscriberLockTime(subAfterAccessTimes, subAfterReleaseTimes); 

	this->pubLatencies = calculator.getLatencies(pubBeforeAccessTimes, pubAfterAccessTimes);
	this->avgPubLatency = calculator.getAvgTime(pubLatencies); 
	this->maxPubLatency = calculator.getMaxTime(pubLatencies); 
	this->minPubLatency = calculator.getMinTime(pubLatencies); 

	this->subLatencies = calculator.getLatencies(subBeforeAccessTimes, subAfterAccessTimes);
	this->avgSubLatencies = calculator.getAvgTimes(subLatencies);
	this->maxSubLatencies = calculator.getMinTimes(subLatencies);
	this->minSubLatencies = calculator.getMaxTimes(subLatencies);
}
