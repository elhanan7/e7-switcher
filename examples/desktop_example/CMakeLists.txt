cmake_minimum_required(VERSION 3.10)
project(e7-switcher-desktop-example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define platform
add_definitions(-D E7_PLATFORM_DESKTOP)

# Source files
set(SOURCES src/main.cpp)

# Create executable
add_executable(e7-switcher-desktop-example ${SOURCES})

# Link with the library
target_link_libraries(e7-switcher-desktop-example PRIVATE e7-switcher)
