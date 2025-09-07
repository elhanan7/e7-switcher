cmake_minimum_required(VERSION 3.18)
project(e7_switcher_py LANGUAGES CXX)

# --- Locate repo root: works both in-repo and in-sdist builds ---
# In-repo layout:   this/CMakeLists.txt, sources in ../src and ../include
# In-sdist layout:  this/CMakeLists.txt, sources in ./src and ./include
set(_TRY_PARENT "${CMAKE_CURRENT_LIST_DIR}/..")
set(_TRY_HERE   "${CMAKE_CURRENT_LIST_DIR}")

if (EXISTS "${_TRY_PARENT}/src/base64_decode.cpp")
  set(REPO_ROOT "${_TRY_PARENT}")
elseif (EXISTS "${_TRY_HERE}/src/base64_decode.cpp")
  set(REPO_ROOT "${_TRY_HERE}")
else()
  message(FATAL_ERROR
    "Could not find src/ next to or above ${CMAKE_CURRENT_LIST_DIR}. "
    "Make sure sdist.include contains src/** and include/**.")
endif()

# ---- Python & pybind11 ----
find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
  include(FetchContent)
  FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v3.0.1
  )
  FetchContent_MakeAvailable(pybind11)
endif()

# ---- Core C++ library ----
# Reuse an existing core if parent provided it; otherwise build it here.
if (TARGET e7-switcher)
  set(CORE_TARGET e7-switcher)
else()
  add_library(e7switcher STATIC
    ${REPO_ROOT}/src/base64_decode.cpp
    ${REPO_ROOT}/src/compression.cpp
    ${REPO_ROOT}/src/crc.cpp
    ${REPO_ROOT}/src/crypto.cpp
    ${REPO_ROOT}/src/data_structures.cpp
    ${REPO_ROOT}/src/e7_switcher_client.cpp
    ${REPO_ROOT}/src/json_helpers.cpp
    ${REPO_ROOT}/src/logger.cpp
    ${REPO_ROOT}/src/message_stream.cpp
    ${REPO_ROOT}/src/messages.cpp
    ${REPO_ROOT}/src/oge_ir_device_code.cpp
    ${REPO_ROOT}/src/parser.cpp
    ${REPO_ROOT}/src/time_utils.cpp
  )
  target_include_directories(e7switcher PUBLIC ${REPO_ROOT}/include)
  target_compile_features(e7switcher PUBLIC cxx_std_17)
  set_target_properties(e7switcher PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set(CORE_TARGET e7switcher)
endif()

# ---- Python extension (_core) ----
pybind11_add_module(_core MODULE
  ${CMAKE_CURRENT_LIST_DIR}/src/bindings.cpp
)
target_link_libraries(_core PRIVATE ${CORE_TARGET})
target_compile_features(_core PRIVATE cxx_std_17)

if (MSVC)
  target_compile_definitions(_core PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  set_target_properties(_core PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
  )
endif()

# rpath so the extension can find any side libs next to itself
if(APPLE)
  set_target_properties(_core PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_RPATH "@loader_path")
elseif(UNIX)
  set_target_properties(_core PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_RPATH "\$ORIGIN")
endif()

# ---- Install into wheel (picked up by scikit-build-core) ----
install(TARGETS _core
  LIBRARY DESTINATION e7_switcher
  RUNTIME DESTINATION e7_switcher
  ARCHIVE DESTINATION e7_switcher
)

# If setuptools_scm wrote version.py, include it
install(CODE "
  if(EXISTS \"${CMAKE_CURRENT_LIST_DIR}/e7_switcher/version.py\")
    file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/e7_switcher\" TYPE FILE
         FILES \"${CMAKE_CURRENT_LIST_DIR}/e7_switcher/version.py\")
  endif()
")
