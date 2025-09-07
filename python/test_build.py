#!/usr/bin/env python3
"""
Simple test script to verify that the Python bindings can be imported.
This doesn't connect to any actual devices, just tests the import.
"""

try:
    print("Attempting to import e7_switcher...")
    import e7_switcher
    print(f"Successfully imported e7_switcher version: {e7_switcher.__version__}")
    print("Available classes and enums:")
    print(f"  - {', '.join(e7_switcher.__all__)}")
    print("\nImport test successful!")
except ImportError as e:
    print(f"Import failed: {e}")
    print("Make sure the package is installed correctly.")
    exit(1)
