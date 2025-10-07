cmake_minimum_required(VERSION 3.20)

project(CSVTableProject LANGUAGES CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header files (for IDEs and dependency tracking)
set(HEADER_FILES
    CSVTable.hpp
)

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Find Apache Arrow and Parquet
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

# Compiler flags
add_compile_options(-Wall -Wextra -O0 -ggdb)

# Executable for Example_uint64.cpp
add_executable(examples_uint64 Example_uint64.cpp ${HEADER_FILES})
target_include_directories(examples_uint64 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(examples_uint64 PRIVATE arrow_shared parquet_shared)

# Executable for IterateExamples.cpp
add_executable(examples_iterate IterateExamples.cpp ${HEADER_FILES})
target_include_directories(examples_iterate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(examples_iterate PRIVATE arrow_shared parquet_shared)

# Executable for CSVTableExamples.cpp
add_executable(examples_general CSVTableExamples.cpp ${HEADER_FILES})
target_include_directories(examples_general PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(examples_general PRIVATE arrow_shared parquet_shared)

# Executable for ParquetExamples.cpp
add_executable(examples_parquet ParquetExamples.cpp ${HEADER_FILES})
target_include_directories(examples_parquet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(examples_parquet PRIVATE arrow_shared parquet_shared)

# Executable for CSVTableTests.cpp (with Google Test)
add_executable(tests CSVTableTests.cpp ${HEADER_FILES})
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests PRIVATE GTest::GTest GTest::Main arrow_shared parquet_shared)

# Enable testing
enable_testing()
add_test(NAME UnitTests COMMAND tests)

# Custom target for cleaning CSV files
add_custom_target(clean-csv
    COMMAND ${CMAKE_COMMAND} -E remove *.csv
    COMMENT "Removing generated CSV files"
)

# Ensure clean-csv is part of the clean target
set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "*.csv")