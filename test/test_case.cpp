#include "test_case.h"
#include <algorithm>
#include <numeric>

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
		this->subAfterAccessTimes.push_back(std::vector<long long>());
		this->subAfterReleaseTimes.push_back(std::vector<long long>());
	}
}

void TestCase::pushToPubAfterAccessTimes(long long time)
{
	this->pubAfterAccessTimes.push_back(time);
}

void TestCase::pushToPubAfterReleaseTimes(long long time)
{
	this->pubAfterReleaseTimes.push_back(time);
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

//metric methods

void TestCase::calculateMetrics()
{
	this->subscriberLockTimes = getSubscriberLockTimes();
	this->iterationLockTimes = getIterationLockTimes();
	this->iterationDurations = getIterationDurations();

	this->avgIterationLockTime = std::accumulate(this->iterationLockTimes.begin(), this->iterationLockTimes.end(), 0LL) / iterationLockTimes.size();
	this->maxIterationLockTime = *std::max_element(this->iterationLockTimes.begin(), this->iterationLockTimes.end());
	this->minIterationLockTime = *std::min_element(this->iterationLockTimes.end(), this->iterationLockTimes.end());

	this->avgSubscriberIterationLockTime = std::accumulate(this->subscriberLockTimes.begin(), this->subscriberLockTimes.end(), 0LL) / this->subscriberLockTimes.size();
	this->maxSubscriberIterationLockTime = *std::max_element(this->subscriberLockTimes.begin(), this->subscriberLockTimes.end());
	this->minSubscriberIterationLockTime = *std::min_element(this->subscriberLockTimes.begin(), this->subscriberLockTimes.end());

	this->avgIterationDuration = std::accumulate(this->iterationLockTimes.begin(), this->iterationLockTimes.end(), 0LL) / this->iterationLockTimes.size();
	this->maxIterationDuration = *std::max_element(this->iterationLockTimes.begin(), this->iterationLockTimes.end());
	this->minIterationDuration = *std::min_element(this->iterationLockTimes.begin(), this->iterationLockTimes.end());

	this->totalDuration = std::accumulate(this->iterationDurations.begin(), this->iterationDurations.end(), 0LL);
	this->totalLockTime = getTotalLockTime();
	this->totalSubcriberLockTime = std::accumulate(this->subscriberLockTimes.begin(), this->subscriberLockTimes.end(), 0LL);
}

std::vector<long long> TestCase::getSubscriberLockTimes() 
{
	std::vector<long long> lockTimes;
	for (int i = 0; i < this->msgCount; i++) {
		lockTimes.push_back(getMinSubAfterAccessTimeForIteration(i) - getMaxSubAfterReleaseTimeForIteration(i));
	}
	return lockTimes;
}

std::vector<long long> TestCase::getIterationLockTimes() 
{
	std::vector<long long> lockTimes;
	return lockTimes;
}

std::vector<long long> TestCase::getIterationDurations()
{
	std::vector<long long> durations;
	for (int i = 0; i < this->msgCount; i++) {
		durations.push_back(getMaxSubAfterReleaseTimeForIteration(i) - this->pubAfterAccessTimes[i]);
	}
	return durations;
}

long long TestCase::getTotalDuration() 
{
	long long maxTime = getMaxSubAfterReleaseTimeForIteration(this->subAfterReleaseTimes.size() - 1);
	return maxTime;
}

long long TestCase::getTotalLockTime() 
{
	long long totalSubLockTime = std::accumulate(this->subscriberLockTimes.begin(), this->subscriberLockTimes.end(), 0LL);
	long long totalPubLockTime = 0;
	for (int i = 0; i < this->msgCount; i++){
		totalPubLockTime += this->pubAfterReleaseTimes[i] - this->pubAfterAccessTimes[i];
	}
	return totalSubLockTime + totalPubLockTime;
}

long long TestCase::getTotalSubscriberLockTime() 
{
	long long totalSubscriberLockTime = 0;
	for (int i = 0; i < this->msgCount; i++) {
		totalSubscriberLockTime += getMaxSubAfterReleaseTimeForIteration(i) - getMinSubAfterAccessTimeForIteration(i);
	}
	return totalSubscriberLockTime; 
}

//metric help methods
long long TestCase::getMinSubAfterAccessTimeForIteration(int iteration)
{
	long long minTime = -1;
	for (int i = 0; i < this->subAfterAccessTimes.size(); i++) {
		long long currTime = this->subAfterAccessTimes[i][iteration];
		if (minTime == -1) {
			minTime = currTime;
			continue;
		}
		if (currTime < minTime) {
			minTime = currTime;
		}
	}
	return minTime;
}

long long TestCase::getMaxSubAfterReleaseTimeForIteration(int iteration)
{
	long long maxTime = 0;

	for (int i = 0; i < this->subAfterReleaseTimes.size(); i++) {
		long long currTime = this->subAfterReleaseTimes[i][iteration];
		if (currTime > maxTime) {
			maxTime = currTime;
		}
	}
	return maxTime;
}


