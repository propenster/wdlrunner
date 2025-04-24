cmake_minimum_required(VERSION 3.10)

# Project name and version
project(wdlrunner VERSION 1.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Add include directory
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add source files
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})
