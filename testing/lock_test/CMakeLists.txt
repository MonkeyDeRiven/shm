cmake_minimum_required(VERSION 3.20)

project(test_locking)

find_package(GTest REQUIRED)

add_executable(rw_lock_test ${CMAKE_CURRENT_SOURCE_DIR}/src/rw_lock_test.cpp)

target_include_directories(rw_lock_test PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(rw_lock_test PRIVATE shm GTest::gtest GTest::gtest_main)

add_test(
    NAME              RwLock 
    COMMAND           rw_lock_test.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)



