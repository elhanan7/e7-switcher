cmake_minimum_required(VERSION 3.10)
project(e7-switcher VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build as shared library
option(BUILD_SHARED_LIBS "Build as shared library" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

# Platform detection and configuration
if(DEFINED ESP_PLATFORM)
    add_definitions(-D E7_PLATFORM_ESP)
else()
    add_definitions(-D E7_PLATFORM_DESKTOP)
    
    # Find required packages for desktop builds
    find_package(OpenSSL REQUIRED)
    find_package(nlohmann_json 3.11.2 REQUIRED)
    find_package(ZLIB REQUIRED)
endif()

# Include directories
include_directories(include)

# Source files - exclude main files which are now in examples
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
list(FILTER SOURCES EXCLUDE REGEX ".*main_.*\.cpp$")

# Create library
if(BUILD_SHARED_LIBS)
    add_library(e7-switcher SHARED ${SOURCES})
else()
    add_library(e7-switcher STATIC ${SOURCES})
endif()

# Set include directories for users of the library
target_include_directories(e7-switcher PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link libraries for desktop builds
if(NOT DEFINED ESP_PLATFORM)
    target_link_libraries(e7-switcher 
        PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        ZLIB::ZLIB
    )
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS e7-switcher
    EXPORT e7-switcher-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export targets
install(EXPORT e7-switcher-targets
    FILE e7-switcher-targets.cmake
    NAMESPACE e7-switcher::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/e7-switcher
)

# Create and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/e7-switcher-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/e7-switcher-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/e7-switcher
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/e7-switcher-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/e7-switcher-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/e7-switcher-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/e7-switcher
)

# Add examples if requested
if(BUILD_EXAMPLES)
    add_subdirectory(examples/desktop_example)
endif()
