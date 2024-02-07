#include "gtest/gtest.h"

#include "metric_calculator.h"

MetricCalculator calc;

TEST(MetricCalc, getAvgTime)
{
	std::vector<long long> times{ 10,5,28,6,4 };
	EXPECT_FLOAT_EQ(calc.getAvgTime(times), 10.6);
}

TEST(MetricCalc, getMaxTime) 
{
	std::vector<long long> times{ 10,5,20,6,4 };
	EXPECT_EQ(calc.getMaxTime(times), 20);
}

TEST(MetricCalc, getMinTime)
{
	std::vector<long long> times{ 10,5,20,6,4 };
	EXPECT_EQ(calc.getMinTime(times), 4);
}

TEST(MetricCalc, getIterationTimes)
{
	std::vector<std::vector<long long>> times{
		{ 10,5,20,6,4 },
		{	9,50,11,13,7},
		{ 31,7,2,15,5 },
		{ 5,11,19,3,10}
	};

	EXPECT_EQ(calc.getIterationTimes(times, 1), std::vector<long long>({ 5,50,7,11 }));
	EXPECT_EQ(calc.getIterationTimes(times, 3), std::vector<long long>({ 6,13,15,3 }));
}

TEST(MetricCalc, getAvgTimes)
{
	std::vector<std::vector<long long>> times{
		{ 10,5,20,6,4 },
		{	9,50,11,13,7},
		{ 31,7,2,15,5 },
		{ 5,11,19,3,10}
	};
	
	EXPECT_EQ(calc.getAvgTimes(times), std::vector<float>({ 13.75, 18.25, 13, 9.25, 6.5 }));
}

TEST(MetricCalc, getMaxTimes)
{
	std::vector<std::vector<long long>> times{
		{ 10,5,20,6,4 },
		{	9,50,11,13,7},
		{ 31,7,2,15,5 },
		{ 5,11,19,3,10}
	};
	
	EXPECT_EQ(calc.getMaxTimes(times), std::vector<long long>({ 31, 50, 20, 15, 10 }));
}

TEST(MetricCalc, getMinTimes)
{
	std::vector<std::vector<long long>> times{
		{ 10,5,20,6,4 },
		{	9,50,11,13,7},
		{ 31,7,2,15,5 },
		{ 5,11,19,3,10}
	};

	EXPECT_EQ(calc.getMinTimes(times), std::vector<long long>({ 5, 5, 2, 3, 4 }));
}

TEST(MetricCalc, getSubscriberLockTimes)
{
	// Every row in the matrix are the afterAccessTimes for one subscriber.
	// The number of elements in it is the number of messages recieved (lock was aquired once for every message)
	std::vector<std::vector<long long>> afterAccessTimes = {
		{ 10,5,20,6,4 },
		{	9,50,11,13,7},
		{ 31,7,2,15,5 },
		{ 5,11,19,3,10}
	};
  
	std::vector<std::vector<long long>> afterReleaseTimes = afterAccessTimes;
	// The expected lockTimes are not the exact times each subscriber held the lock for added up,
	// because it would not show the difference between the mutex and rw-lock. This is due to the fact,
	// that the lock type does not change how long a certain process holds a lock. If sub A holds the lock for 5ms
	// using a read write lock, it will also hold the rw-lock for 5ms. The difference is that with a mutex,
	// the 5 milliseconds add up, because they have to lock it after each other. With the rw-lock, each sub still holds
	// the lock for 5ms, but they do it simultaniously so it is not adding up. Thats why the function takes the time stamp,
	// of the sub who aquired the lock first and subtracts it form the time stamp of the sub which released it last.
	// This makes the expected output for a iteration the highest value in the afterAccessTimes list - the lowest value int this list
	// Example iteration 1: 31-5 = 26
	EXPECT_EQ(calc.getSubscriberLockTimes(afterAccessTimes, afterReleaseTimes), std::vector<long long>({ 26, 45, 18, 12, 6 }));
}

TEST(MetricCalc, getIterationLockTimes) 
{
	std::vector<long long> subscriberLockTimes = { 26, 45, 18, 12, 6 };
	// pub held the lock for 3ms for every message
	std::vector<long long> pubAfterAccessTimes = { 3, 6, 9, 12, 15 };
	std::vector<long long> pubAfterReleaseTimes = { 6, 9, 12, 15, 18 };

	EXPECT_EQ(calc.getIterationLockTimes(subscriberLockTimes, pubAfterAccessTimes, pubAfterReleaseTimes), std::vector<long long>({ 29, 48, 21, 15, 9 }));
}

TEST(MetricCalc, getIterationDuration)
{
	std::vector<long long> pubAfterAccessTimes =	{ 0, 12, 24, 36, 48 };
	std::vector<std::vector<long long>> subAfterReleaseTimes = { 
		{5, 14, 35, 47, 55 },
		{11, 18, 27, 42, 50 },
		{9, 23, 32, 40, 59 } 
	};

	EXPECT_EQ(calc.getIterationDurations(pubAfterAccessTimes, subAfterReleaseTimes), std::vector<long long>({ 11, 11, 11, 11, 11 }));
}

TEST(MetricCalc, getLatencies) 
{
	// for publisher
	{
		std::vector<long long> beforeAccessTimes = { 3, 6, 9, 12, 15 };
		std::vector<long long> afterAccessTimes = { 4, 8, 12, 14, 16 };

		EXPECT_EQ(calc.getLatencies(beforeAccessTimes, afterAccessTimes), std::vector<long long>({ 1, 2, 3, 2, 1 })) << "getLatencies for publisher failed";
	}

	// for subscriber
	std::vector<std::vector<long long>> beforeAccessTimes = { 
		{ 3, 6, 9, 12, 15 },
		{ 13, 16, 19, 22, 25 } 
	};
	std::vector<std::vector<long long>> afterAccessTimes = {
		{ 4, 8, 12, 14, 16 },
		{ 15, 20, 25, 26, 27}
	};

	std::vector<std::vector<long long>> latencies = {
		{ 1, 2 },
		{ 2, 4 },
		{ 3, 6 },
		{ 2, 4 },
		{ 1, 2 }
	};

	// for subscriber
	EXPECT_EQ(calc.getLatencies(beforeAccessTimes, afterAccessTimes), latencies) << "getLatencies for publisher failed";
} 

TEST(MetricCalc, getTotalDuration)
{
	std::vector<std::vector<long long>> subAfterReleaseTimes = {
	{ 4, 8, 12, 14, 16 },
	{ 15, 20, 25, 26, 27},
	{ 7, 4, 21, 22, 25 }
	};

	EXPECT_EQ(calc.getTotalDuration(subAfterReleaseTimes), 27);
}

TEST(MetricCalc, getTotalSubscriberLockTime)
{
	std::vector<std::vector<long long>> afterAccessTimes = {
	{ 1, 5, 9, 13, 17 },
	{ 2, 6, 10, 14, 18 },
	{ 3, 7, 11, 15, 19 }
	};
	std::vector<std::vector<long long>> afterReleaseTimes = {
		{ 2, 6, 10, 14, 18 },
		{ 3, 7, 11, 15, 19 },
		{ 4, 8, 12, 16, 20 }
	};

	EXPECT_EQ(calc.getTotalSubscriberLockTime(afterAccessTimes, afterReleaseTimes), 15);
}

TEST(MetricCalc, getTotalLockTime) 
{
	std::vector<long long> subLockTimes = { 3, 4, 3, 6, 2 };

	std::vector<long long> pubAfterAccess = { 0, 10, 15, 27, 33 };
	std::vector<long long> pubAfterRelease = { 2, 13, 20, 30, 37 };

	EXPECT_EQ(calc.getTotalLockTime(subLockTimes, pubAfterAccess, pubAfterRelease), 35);
}
