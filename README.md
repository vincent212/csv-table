# CSVTableProject

CSVTableProject is a C++ project that provides a convenient way to read, manipulate, and write CSV files using a table-like structure. The core of the project is the `CSVTable` class, which allows users to work with CSV data in a flexible and type-safe manner.

## Features

- Read CSV files with automatic type inference for cell values
- Access and modify cell values using row indices and column names
- Support for various data types including strings adddddd, integers, doubles, booleans, and uint64_t
- Iterate over rows and columns easily
- Manipulate table structure by adding or removing columns
- Apply functions to entire columns
- Handle missing values and perform data cleaning operations

For a detailed list of features, see [features.md](features.md).

## Getting Started

To build and run the project, follow the instructions in [build_instructions.md](build_instructions.md). This document provides step-by-step guidance on setting up the build environment, compiling the code, and running the examples and tests.

### Prerequisites

- CMake (version 3.20 or higher)
- C++23-compatible compiler (e.g., GCC 13 or later)
- Google Test for unit testing

## Examples

The project includes several example files to demonstrate the usage of `CSVTable`:

- `Example_uint64.cpp`: Shows how to work with `uint64_t` data types in the table
- `CSVTableExamples.cpp`: Provides general usage examples of `CSVTable`
- `IterateExamples.cpp`: Demonstrates various ways to iterate over the table's rows and columns

To run these examples, build the project and execute the corresponding executables as described in [build_instructions.md](build_instructions.md).

## Testing

Unit tests are provided in `CSVTableTests.cpp` and use Google Test. To run the tests, build the project and execute the `tests` executable. For more information, see [build_instructions.md](build_instructions.md).

## Comparison with pandas

For users familiar with Python's pandas library, [pandascomparison.md](pandascomparison.md) provides a comparison between `CSVTable` and pandas, highlighting similarities and differences in functionality and usage.

## License

[Add license information here, if applicable]