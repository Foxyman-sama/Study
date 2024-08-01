cmake_minimum_required(VERSION 3.29)
project(protobuf_tests)

set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest REQUIRED)
find_package(Protobuf REQUIRED)

file(GLOB sources ${PROJECT_SOURCE_DIR}/*.cxx)
add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME}  
    GTest::gtest_main
    GTest::gmock_main
    protobuf::libprotoc protobuf::libprotobuf)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# BUILD ONLY WITH CMAKE!!!
protobuf_generate(TARGET ${PROJECT_NAME} PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/hello.proto)

add_test(${PROJECT_NAME} ${PROJECT_NAME})



