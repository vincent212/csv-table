# Build Instructions for CSVTableProject

This document provides step-by-step instructions to build and run the `CSVTableProject`, a C++ project that processes CSV files using the `CSVTable` class. The project includes example programs and unit tests, built using CMake and requiring C++23 and Google Test.

## Prerequisites

Before building the project, ensure the following tools and libraries are installed on your system:

- **CMake**: Version 3.20 or higher.
  - Install on Ubuntu:
    ```bash
    sudo apt update
    sudo apt install cmake
    ```
- **C++ Compiler**: A C++23-compatible compiler (e.g., GCC 13 or later).
  - Install GCC on Ubuntu:
    ```bash
    sudo apt install g++-13
    ```
    Ensure `g++-13` is used by setting:
    ```bash
    export CXX=g++-13
    ```
- **Google Test**: Required for unit tests.
  - Install on Ubuntu:
    ```bash
    sudo apt install libgtest-dev
    ```
    Note: You may need to build Google Test manually if the packaged version is outdated:
    ```bash
    sudo apt install libgtest-dev
    cd /usr/src/gtest
    sudo cmake .
    sudo make
    sudo cp *.a /usr/lib
    ```
- **Make**: Typically pre-installed on Linux, used to execute the build.
  - Verify installation:
    ```bash
    make
    