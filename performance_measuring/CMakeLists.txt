add_executable(performance_measuring)

target_sources(performance_measuring
  PRIVATE
    main.cpp
)

add_library(performance_test     
    test_case.cpp
    test_case.h
    test_case_copy.cpp
    test_case_copy.h
    test_case_zero_copy.cpp
    test_case_zero_copy.h)

protobuf_target_cpp(performance_test ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test_case.proto)
protobuf_target_cpp(performance_measuring ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test_case.proto)

target_link_libraries(performance_measuring PRIVATE shm performance_test protobuf::libprotobuf)
target_link_libraries(performance_test PRIVATE protobuf::libprotobuf)
