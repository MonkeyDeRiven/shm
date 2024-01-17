add_executable(test)

target_sources(test
  PRIVATE
    main.cpp
    test_case.cpp
    test_case.h
    test_case_copy.cpp
    test_case_copy.h
    test_case_zero_copy.cpp
    test_case_zero_copy.h
)

protobuf_target_cpp(test ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test_case.proto)

target_link_libraries(test PRIVATE shm protobuf::libprotobuf)

