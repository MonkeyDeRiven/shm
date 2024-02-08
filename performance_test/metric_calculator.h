#pragma once

#include <vector>

class MetricCalculator {
public: 

	float getAvgTime(std::vector<long long>& times);
	std::vector<float> getAvgTimes(std::vector<std::vector<long long>>& times);
	long long getMaxTime(std::vector<long long>& times);
	std::vector<long long> getMaxTimes(std::vector<std::vector<long long>>& times);
	long long getMinTime(std::vector<long long>& times);
	std::vector<long long> getMinTimes(std::vector<std::vector<long long>>& times);

	std::vector<long long> getSubscriberLockTimes(std::vector<std::vector<long long>>& subAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes);
	std::vector<long long> getIterationLockTimes(std::vector<long long>& subscriberLockTimes, std::vector<long long>& pubAfterAccessTimes, std::vector<long long>& pubAfterReleaseTimes);
	std::vector<long long> getIterationDurations(std::vector<long long>& pubAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes);

	long long getTotalLockTime(std::vector<long long>& subscriberLockTimes, std::vector<long long>& pubAfterAccessTimes, std::vector<long long>& pubAfterReleaseTimes);

	long long getTotalSubscriberLockTime(std::vector<std::vector<long long>>& subAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes);

	std::vector<long long> getLatencies(std::vector<long long>& beforeAccessTimes, std::vector<long long>& afterAccessTimes);
	std::vector<std::vector<long long>> getLatencies(std::vector<std::vector<long long>>& beforeAccessTimes, std::vector<std::vector<long long>>& afterAccessTimes);
	long long getTotalDuration(std::vector<std::vector<long long>>& subAfterReleaseTimes);

	std::vector<long long> getIterationTimes(std::vector<std::vector<long long>>& times, int iteration);

};