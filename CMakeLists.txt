cmake_minimum_required(VERSION 3.10)
project(e7_switcher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define platform
add_definitions(-D E7_PLATFORM_DESKTOP)

# Include directories
include_directories(include)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.11.2 REQUIRED)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(e7_switcher ${SOURCES})

# Link libraries
target_link_libraries(e7_switcher 
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)
