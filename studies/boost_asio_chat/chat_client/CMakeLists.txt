cmake_minimum_required(VERSION 3.29)
project(chat_client)

set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 26)

add_executable(${PROJECT_NAME} main.cc)
