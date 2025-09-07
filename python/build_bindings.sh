#!/bin/bash
# Script to build the Python bindings for E7 Switcher

set -e  # Exit on error

# Parse arguments
PYTHON_VERSION=""
BUILD_TYPE="Release"
INSTALL=0

print_usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -p, --python-version VERSION  Specify Python version (e.g., 3.9)"
    echo "  -d, --debug                   Build in debug mode"
    echo "  -i, --install                 Install the package after building"
    echo "  -h, --help                    Show this help message"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--python-version)
            PYTHON_VERSION="$2"
            shift 2
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -i|--install)
            INSTALL=1
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Create build directory
mkdir -p build
cd build

# Configure with CMake
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

if [ -n "$PYTHON_VERSION" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DPYTHON_VERSION=$PYTHON_VERSION"
    echo "Using Python version: $PYTHON_VERSION"
else
    echo "Using default Python version"
fi

echo "Configuring with CMake..."
cmake .. $CMAKE_ARGS

# Build
echo "Building..."
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

# Install if requested
if [ $INSTALL -eq 1 ]; then
    echo "Installing..."
    cd ..
    if [ -n "$PYTHON_VERSION" ]; then
        PYTHON_VERSION=$PYTHON_VERSION pip install .
    else
        pip install .
    fi
fi

echo "Build completed successfully!"
