cmake_minimum_required(VERSION 3.29)
project(chat_server)

find_package(Boost REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 26)

add_executable(${PROJECT_NAME} main.cc)
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
