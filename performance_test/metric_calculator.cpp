#include "metric_calculator.h"
#include <algorithm>
#include <numeric>

float MetricCalculator::getAvgTime(const std::vector<long long>& times)
{
	return float(std::accumulate(times.begin(), times.end(), 0LL)) / float(times.size());
}
long long MetricCalculator::getMaxTime(const std::vector<long long>& times)
{
	return *std::max_element(times.begin(), times.end());
}
long long MetricCalculator::getMinTime(const std::vector<long long>& times)
{
	return *std::min_element(times.begin(), times.end());
}

std::vector<float> MetricCalculator::getAvgTimes(std::vector<std::vector<long long>>& times)
{
	std::vector<float> avgs;
	for (int i = 0; i < times[0].size(); i++)
		avgs.push_back(getAvgTime(getIterationTimes(times, i)));
	return avgs;
}
std::vector<long long> MetricCalculator::getMaxTimes(std::vector<std::vector<long long>>& times)
{
	std::vector<long long> maxs;
	for (int i = 0; i < times[0].size(); i++)
		maxs.push_back(getMaxTime(getIterationTimes(times, i)));
	return maxs;
}
std::vector<long long> MetricCalculator::getMinTimes(std::vector<std::vector<long long>>& times)
{
	std::vector<long long> mins;
	for (int i = 0; i < times[0].size(); i++)
		mins.push_back(getMinTime(getIterationTimes(times, i)));
	return mins;
}


std::vector<long long> MetricCalculator::getSubscriberLockTimes(std::vector<std::vector<long long>>& subAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes)
{
	std::vector<long long> lockTimes;
	for (int i = 0; i < subAfterReleaseTimes[0].size() ; i++) {
		lockTimes.push_back(getMaxTime(getIterationTimes(subAfterReleaseTimes, i)) - getMinTime(getIterationTimes(subAfterAccessTimes, i)));
	}
	return lockTimes;
}

std::vector<long long> MetricCalculator::getIterationLockTimes(std::vector<long long>& subLockTimes, std::vector<long long>& pubAfterAccessTimes, std::vector<long long>& pubAfterReleaseTimes)
{
	std::vector<long long> lockTimes;
	for (int i = 0; i < pubAfterAccessTimes.size(); i++) {
		lockTimes.push_back(subLockTimes[i] + (pubAfterReleaseTimes[i] - pubAfterAccessTimes[i]));
	}
	return lockTimes;
}

std::vector<long long> MetricCalculator::getIterationDurations(std::vector<long long>& pubAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes)
{
	std::vector<long long> durations;
	for (int i = 0; i < pubAfterAccessTimes.size(); i++) {
		durations.push_back(getMaxTime(getIterationTimes(subAfterReleaseTimes, i)) - pubAfterAccessTimes[i]);
	}
	return durations;
}

long long MetricCalculator::getTotalLockTime(std::vector<std::vector<long long>>& subAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes, std::vector<long long>& pubAfterAccessTimes, std::vector<long long>& pubAfterReleaseTimes)
{
	long long totalSubLockTime = getTotalSubscriberLockTime(subAfterAccessTimes, subAfterReleaseTimes);
	long long totalPubLockTime = 0;
	for (int i = 0; i < pubAfterAccessTimes.size(); i++) {
		totalPubLockTime += pubAfterReleaseTimes[i] - pubAfterAccessTimes[i];
	}
	return totalSubLockTime + totalPubLockTime;
}

std::vector<long long> MetricCalculator::getLatencies(const std::vector<long long>& beforeAccessTimes, const std::vector<long long>& afterAccessTimes)
{
	std::vector<long long> latencies;

	for (int i = 0; i < beforeAccessTimes.size(); i++) {
		latencies.push_back(afterAccessTimes[i] - beforeAccessTimes[i]);
	}
	return latencies;
}

std::vector<std::vector<long long>> MetricCalculator::getLatencies(std::vector<std::vector<long long>>& beforeAccessTimes, std::vector<std::vector<long long>>& afterAccessTimes)
{
	std::vector<std::vector<long long>> latencies;

	for (int i = 0; i < beforeAccessTimes[0].size(); i++)
	{
		latencies.push_back(getLatencies(getIterationTimes(beforeAccessTimes, i), getIterationTimes(afterAccessTimes, i)));
	}
	return latencies;
}

long long MetricCalculator::getTotalDuration(std::vector<std::vector<long long>>& subAfterReleaseTimes)
{
	return getMaxTime(getIterationTimes(subAfterReleaseTimes, subAfterReleaseTimes[0].size() - 1));
}

long long MetricCalculator::getTotalSubscriberLockTime(std::vector<std::vector<long long>>& subAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes)
{
	long long totalSubscriberLockTime = 0;
	for (int i = 0; i < subAfterAccessTimes[0].size(); i++) {
		totalSubscriberLockTime += getMaxTime(getIterationTimes(subAfterReleaseTimes, i)) - getMinTime(getIterationTimes(subAfterAccessTimes, i));
	}
	return totalSubscriberLockTime;
}

std::vector<long long> MetricCalculator::getIterationTimes(const std::vector<std::vector<long long>>& times, int iteration) {
	std::vector<long long> iterationTimes;
	for (int i = 0; i < times.size(); i++)
		iterationTimes.push_back(times[i][iteration]);
	return iterationTimes;
}