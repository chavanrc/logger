cmake_minimum_required(VERSION 3.19)

project(Logger)

set(CMAKE_CXX_STANDARD 20)

find_package(spdlog REQUIRED)

add_executable(Logger main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)