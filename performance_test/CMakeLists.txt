add_library(performance_test)

target_sources(performance_test
  PUBLIC
    test_case.h
    test_case_copy.h
    test_case_zero_copy.h
    metric_calculator.h
  PRIVATE
    test_case.cpp
    test_case_copy.cpp
    test_case_zero_copy.cpp
    metric_calculator.cpp)

target_include_directories(performance_test 
  PUBLIC 
    .
  PRIVATE
    ${Protobuf_INCLUDE_DIRS})

protobuf_target_cpp(performance_test ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test_case.proto)
target_link_libraries(performance_test PRIVATE protobuf::libprotobuf)

