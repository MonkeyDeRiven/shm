cmake_minimum_required(VERSION 3.20)

project(ecal_locking)

option(ECAL_THIRDPARTY_BUILD_PROTOBUF        "Build protobuf with eCAL"                                          ON)

if (ECAL_THIRDPARTY_BUILD_PROTOBUF)
    include(thirdparty/protobuf/build-protobuf.cmake)
endif ()

include(protoc_generate_cpp.cmake)

add_subdirectory(lib)
add_subdirectory(test)
