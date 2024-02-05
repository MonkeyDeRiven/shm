cmake_minimum_required(VERSION 3.20)

project(test_metrics_calculation)

find_package(GTest REQUIRED)

add_executable(metrics_calculation_test ${CMAKE_CURRENT_SOURCE_DIR}/src/metrics_calculation_test.cpp)

target_include_directories(metrics_calculation_test PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(metrics_calculation_test PRIVATE performance_test shm GTest::gtest GTest::gtest_main)

add_test(
    NAME              metricsTest
    COMMAND           metrics_calculation_test.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)



