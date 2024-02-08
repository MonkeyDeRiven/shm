cmake_minimum_required(VERSION 3.20)

project(ecal_locking)

option(ECAL_THIRDPARTY_BUILD_PROTOBUF        "Build protobuf with eCAL"                                          ON)
option(ECAL_THIRDPARTY_BUILD_GTEST             "Build gtest with eCAL"                                           ON)

if (ECAL_THIRDPARTY_BUILD_PROTOBUF)
    include(thirdparty/protobuf/build-protobuf.cmake)
endif ()

if (ECAL_THIRDPARTY_BUILD_GTEST)
    include(thirdparty/gtest/build-gtest.cmake)
endif ()

include(protoc_generate_cpp.cmake)

add_subdirectory(lib)
add_subdirectory(performance_test)
add_subdirectory(performance_measuring)

# unit tests
enable_testing()

add_subdirectory(testing/lock_test)
add_subdirectory(testing/metrics_calculation_test)