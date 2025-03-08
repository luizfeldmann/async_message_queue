cmake_minimum_required(VERSION 3.19)

project(PackageTest CXX)

find_package(async_message_queue CONFIG REQUIRED)

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME}
    async_message_queue::async_message_queue )
