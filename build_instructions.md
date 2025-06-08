# Build Instructions for CSVTableProject

This document provides step-by-step instructions to build and run the `CSVTableProject`, a C++ project that processes CSV files using the `CSVTable` class. The project includes example programs and unit tests, built using CMake and requiring C++23 and Google Test. These instructions are tailored for a Linux-based system (e.g., Ubuntu).

## Prerequisites

Before building the project, ensure the following tools and libraries are installed on your system:

- **CMake**: Version 3.20 or higher.
  - Install on Ubuntu:
    ```bash
    sudo apt update
    sudo apt install cmake
    ```
  - Verify the version:
    ```bash
    cmake --version
    ```

- **C++ Compiler**: A C++23-compatible compiler (e.g., GCC 13 or later).
  - Install GCC 13 on Ubuntu:
    ```bash
    sudo apt install g++-13
    ```
  - Set `g++-13` as the default compiler for this session:
    ```bash
    export CXX=g++-13
    ```

- **Google Test**: Required for unit tests.
  - Install on Ubuntu:
    ```bash
    sudo apt install libgtest-dev
    ```
  - If the packaged version is outdated or not properly linked, build it manually:
    ```bash
    cd /usr/src/gtest
    sudo cmake .
    sudo make
    sudo cp *.a /usr/lib
    ```

- **Make**: Used to execute the build after CMake configuration.
  - Verify installation:
    ```bash
    make --version
    ```
  - Install if missing:
    ```bash
    sudo apt install make
    ```

## Setting Up the Build Environment

To organize build artifacts and keep the source directory clean, create a separate build directory:

1. **Navigate to the Project Directory**:
   - Assuming the project is located at `~/csv-table`, run:
     ```bash
     cd ~/csv-table
     ```

2. **Create and Enter the Build Directory**:
   - Create a `build` directory and navigate into it:
     ```bash
     mkdir -p build
     cd build
     ```
   - This separates generated files (e.g., Makefiles, object files) from the source code.

## Running CMake to Configure the Build

CMake generates the build system files (e.g., Makefiles) based on the project’s `CMakeLists.txt`. Follow these steps:

1. **Run the CMake Command**:
   - From the `build` directory, execute:
     ```bash
     cmake ..
     ```
   - **Explanation**:
     - `..` points CMake to the `CMakeLists.txt` in the project root (`~/m2/csv-table`).
     - CMake configures the project, checks for dependencies (e.g., Google Test, C++23 support), and generates build files in the `build` directory.

2. **Expected Output**:
   - A successful run produces output like:
     ```
     -- The CXX compiler identification is GNU 13.2.0
     -- Found GTest: /usr/lib/libgtest.a
     -- Configuring done
     -- Generating done
     -- Build files have been written to: ~/m2/csv-table/build
     ```

3. **Optional Flags** (if needed):
   - Specify the compiler:
     ```bash
     cmake .. -DCMAKE_CXX_COMPILER=g++-13
     ```
   - Enable verbose output:
     ```bash
     cmake .. --verbose
     ```

## Building the Project with Make

After CMake configuration, compile and link the project using Make:

1. **Run the Make Command**:
   - From the `build` directory, run:
     ```bash
     make
     ```
   - This compiles the source files and links them with Google Test, producing executables like `example_uint64`, `examples`, `iterate_examples`, and `tests`.

2. **Expected Output**:
   - Compilation and linking messages, ending with:
     ```
     [100%] Built target tests
     ```

## Running the Executables and Tests

From the `build` directory, run the generated executables:

- **Run Examples**:
  ```bash
  ./example_uint64
  ./examples
  ./iterate_examples
  ```

- **Run Unit Tests**:
  ```bash
  ./tests
  ```

## Troubleshooting

- **CMake Cannot Find Google Test**:
  - Verify `libgtest-dev` is installed or manually built, then re-run:
    ```bash
    cmake ..
    ```

- **Compiler Errors**:
  - Ensure `g++-13` is set (`export CXX=g++-13`) and re-run CMake and Make.

- **Missing CMakeLists.txt**:
  - Confirm you’re in `build` and the project root (`..`) contains `CMakeLists.txt`.