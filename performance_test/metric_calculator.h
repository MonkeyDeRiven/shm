#pragma once

#include <vector>

class MetricCalculator {
public: 

	static float getAvgTime(const std::vector<long long>& times);
	static std::vector<float> getAvgTimes(std::vector<std::vector<long long>>& times);
	static long long getMaxTime(const std::vector<long long>& times);
	static std::vector<long long> getMaxTimes(std::vector<std::vector<long long>>& times);
	static long long getMinTime(const std::vector<long long>& times);
	static std::vector<long long> getMinTimes(std::vector<std::vector<long long>>& times);

	static std::vector<long long> getSubscriberLockTimes(std::vector<std::vector<long long>>& subAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes);
	static std::vector<long long> getIterationLockTimes(std::vector<long long>& subscriberLockTimes, std::vector<long long>& pubAfterAccessTimes, std::vector<long long>& pubAfterReleaseTimes);
	static std::vector<long long> getIterationDurations(std::vector<long long>& pubAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes);

	static long long getTotalLockTime(std::vector<std::vector<long long>>& subAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes, std::vector<long long>& pubAfterAccessTimes, std::vector<long long>& pubAfterReleaseTimes);
	static long long getTotalSubscriberLockTime(std::vector<std::vector<long long>>& subAfterAccessTimes, std::vector<std::vector<long long>>& subAfterReleaseTimes);

	static std::vector<long long> getLatencies(const std::vector<long long>& beforeAccessTimes, const std::vector<long long>& afterAccessTimes);
	static std::vector<std::vector<long long>> getLatencies(std::vector<std::vector<long long>>& beforeAccessTimes, std::vector<std::vector<long long>>& afterAccessTimes);
	static long long getTotalDuration(std::vector<std::vector<long long>>& subAfterReleaseTimes);

	static std::vector<long long> getIterationTimes(const std::vector<std::vector<long long>>& times, int iteration);

};