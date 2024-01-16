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

target_link_libraries(test PRIVATE shm)
