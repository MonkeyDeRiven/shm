#pragma once

class PerformanceTest {
public:
	PerformanceTest(int, int, int, int);
private:
	int subCount;
	int pubCount;
	int msgCount;
	int payloadSize;
};