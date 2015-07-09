cmake_minimum_required(VERSION 2.8.0)

project(7zipcsdk_demo)

add_subdirectory(7z)

include_directories(.)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(7zipSdkDemo src/main.cpp)

target_link_libraries(7zipSdkDemo 7zdec Shlwapi.lib)
