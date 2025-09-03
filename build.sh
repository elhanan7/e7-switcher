#!/bin/bash
# Build script for Switcher E7 project
# Supports both ESP32 (PlatformIO) and desktop (CMake) builds

set -e

function print_help {
    echo "Usage: ./build.sh [options]"
    echo "Options:"
    echo "  --esp32       Build for ESP32 using PlatformIO"
    echo "  --desktop     Build for desktop using CMake"
    echo "  --debug       Build desktop version with debug symbols (only with --desktop)"
    echo "  --clean       Clean build directories before building"
    echo "  --upload      Upload to ESP32 (only with --esp32)"
    echo "  --monitor     Monitor ESP32 after upload (only with --esp32)"
    echo "  --help        Show this help message"
}

# Default values
BUILD_ESP32=0
BUILD_DESKTOP=0
CLEAN=0
UPLOAD=0
MONITOR=0
DEBUG=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --esp32)
            BUILD_ESP32=1
            shift
            ;;
        --desktop)
            BUILD_DESKTOP=1
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        --upload)
            UPLOAD=1
            shift
            ;;
        --monitor)
            MONITOR=1
            shift
            ;;
        --debug)
            DEBUG=1
            shift
            ;;
        --help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_help
            exit 1
            ;;
    esac
done

# If no build target specified, show help
if [[ $BUILD_ESP32 -eq 0 && $BUILD_DESKTOP -eq 0 ]]; then
    echo "Error: No build target specified."
    print_help
    exit 1
fi

# Build for ESP32
if [[ $BUILD_ESP32 -eq 1 ]]; then
    echo "Building for ESP32..."
    
    # Check if PlatformIO is installed
    if ! command -v pio &> /dev/null; then
        echo "Error: PlatformIO not found. Please install it first."
        exit 1
    fi
    
    # Clean if requested
    if [[ $CLEAN -eq 1 ]]; then
        echo "Cleaning ESP32 build..."
        pio run -t clean
    fi
    
    # Build
    pio run
    
    # Upload if requested
    if [[ $UPLOAD -eq 1 ]]; then
        echo "Uploading to ESP32..."
        pio run -t upload
        
        # Monitor if requested
        if [[ $MONITOR -eq 1 ]]; then
            echo "Starting monitor..."
            pio device monitor
        fi
    fi
fi

# Build for desktop
if [[ $BUILD_DESKTOP -eq 1 ]]; then
    echo "Building for desktop..."
    
    # Check if CMake is installed
    if ! command -v cmake &> /dev/null; then
        echo "Error: CMake not found. Please install it first."
        exit 1
    fi
    
    # Create build directory if it doesn't exist
    mkdir -p build
    cd build
    
    # Clean if requested
    if [[ $CLEAN -eq 1 ]]; then
        echo "Cleaning desktop build..."
        rm -rf *
    fi
    
    # Configure and build
    if [[ $DEBUG -eq 1 ]]; then
        echo "Configuring for debug build..."
        cmake -DCMAKE_BUILD_TYPE=Debug ..
    else
        echo "Configuring for release build..."
        cmake -DCMAKE_BUILD_TYPE=Release ..
    fi
    make
    
    echo "Desktop build complete. Executable is at: build/e7_switcher"
fi

echo "Build process completed successfully!"
