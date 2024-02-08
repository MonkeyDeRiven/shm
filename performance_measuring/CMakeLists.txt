add_executable(performance_measuring)

target_sources(performance_measuring
  PRIVATE
    main.cpp
)

protobuf_target_cpp(performance_measuring ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test_case.proto)

target_link_libraries(performance_measuring PRIVATE shm protobuf::libprotobuf performance_test)


