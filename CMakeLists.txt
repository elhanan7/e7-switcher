cmake_minimum_required(VERSION 3.10)
project(e7-switcher VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_SHARED_LIBS "Build as shared library" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)

# Platform detection and configuration
if(DEFINED ESP_PLATFORM)
    add_definitions(-D E7_PLATFORM_ESP)
else()
    add_definitions(-D E7_PLATFORM_DESKTOP)
    
    # Find required packages for desktop builds
    find_package(OpenSSL REQUIRED)
    # ZLIB: prefer system package, fall back to FetchContent
    find_package(ZLIB QUIET)

# Try to find nlohmann_json first; fall back to FetchContent if not found
find_package(nlohmann_json 3.12.0 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# If ZLIB was not found, fetch and build it
if(NOT ZLIB_FOUND)
    include(FetchContent)
    FetchContent_Declare(zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.2.13
    )
    FetchContent_MakeAvailable(zlib)
    # Provide a consistent imported-like target name for linking
    if(NOT TARGET ZLIB::ZLIB)
        if(TARGET zlib)
            add_library(ZLIB::ZLIB ALIAS zlib)
        elseif(TARGET zlibstatic)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        endif()
    endif()
endif()

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

# Ensure PIC for desktop builds so the static library can link into shared modules (Python extension)
if(NOT DEFINED ESP_PLATFORM)
    set_target_properties(e7-switcher PROPERTIES POSITION_INDEPENDENT_CODE ON)
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

# If nlohmann_json was provided via FetchContent (i.e., not an IMPORTED target),
# add it to our export set so install(EXPORT ...) does not fail.
# Use the real target name (nlohmann_json), not the namespaced alias.
if(TARGET nlohmann_json)
    get_target_property(_NJ_IMPORTED nlohmann_json IMPORTED)
    if(NOT _NJ_IMPORTED)
        install(TARGETS nlohmann_json EXPORT e7-switcher-targets)
    endif()
endif()

# If zlib was provided via FetchContent (i.e., not an IMPORTED target), export it too
set(_ZLIB_REAL_TARGET "")
if(TARGET zlib)
    set(_ZLIB_REAL_TARGET zlib)
elseif(TARGET zlibstatic)
    set(_ZLIB_REAL_TARGET zlibstatic)
endif()
if(_ZLIB_REAL_TARGET)
    get_target_property(_ZLIB_IMPORTED ${_ZLIB_REAL_TARGET} IMPORTED)
    if(NOT _ZLIB_IMPORTED)
        install(TARGETS ${_ZLIB_REAL_TARGET} EXPORT e7-switcher-targets)
    endif()
endif()

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

# Add Python bindings if requested
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()
