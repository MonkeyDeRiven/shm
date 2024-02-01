#include "performance_test.h"

PerformanceTest::PerformanceTest(int subCount, int pubCount, int msgCount, int payloadSize) {
	this->subCount = subCount;
	this->pubCount = pubCount;
	this->msgCount = msgCount;
	this->payloadSize = payloadSize;
}