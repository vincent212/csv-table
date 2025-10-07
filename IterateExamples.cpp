// IterateExamples.cpp
// This file demonstrates various ways to iterate over a CSVTable object using an internal representation.

#include "CSVTable.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <unordered_map>

// Helper function to create an example CSVTable with hard-coded data
m2::CSVTable create_example_table() {
    // Define column names
    std::vector<std::string> col_names = {"Name", "Age", "Date", "Score", "Category"};
    
    // Define row data as strings
    std::vector<std::vector<std::string>> data = {
        {"Alice", "25", "2023-01-01", "85.5", "A"},
        {"Bob", "30", "2023-01-02", "90.0", "B"},
        {"Charlie", "", "2023-01-03", "75.2", "A"},
        {"David", "22", "2023-01-04", "", "C"},
        {"Eve", "28", "2023-01-05", "88.8", "B"}
    };
    
    // Parse string data into CellValue
    std::vector<std::vector<m2::CSVTable::CellValue>> rows;
    for (const auto& row_str : data) {
        std::vector<m2::CSVTable::CellValue> row;
        for (const auto& cell_str : row_str) {
            row.push_back(m2::CSVTable::parse_cell(cell_str));
        }
        rows.push_back(row);
    }
    
    // Create column map
    std::unordered_map<std::string, int, m2::string_hash, m2::string_equal> col_map;
    for (size_t i = 0; i < col_names.size(); ++i) {
        col_map[col_names[i]] = i;
    }
    
    // Return the CSVTable object
    return m2::CSVTable(col_names, col_map, rows);
}

// Example 1: Iterating Over All Rows and Columns
// This example iterates over each row in the table and accesses cell values using column names.
void example1() {
    try {
        m2::CSVTable table = create_example_table();
        for (size_t row = 0; row < table.num_rows(); ++row) {
            std::cout << "Row " << row << ":\n";
            for (const auto& col : table.get_col_names()) {
                try {
                    std::string value = table.get<std::string>(row, col);
                    std::cout << "  " << col << ": " << value << "\n";
                } catch (const std::exception& e) {
                    std::cout << "  " << col << ": Error - " << e.what() << "\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in example1: " << e.what() << "\n";
    }
}

// Example 2: Iterating Over a Specific Column
// This example shows how to iterate over all values in a specific column (e.g., "Age").
void example2() {
    try {
        m2::CSVTable table = create_example_table();
        std::string column_name = "Age";
        try {
            // Check if column exists by attempting to access it
            table.get<int>(0, column_name);
            for (size_t row = 0; row < table.num_rows(); ++row) {
                try {
                    int age = table.get<int>(row, column_name);
                    std::cout << "Row " << row << ", Age: " << age << "\n";
                } catch (const std::exception& e) {
                    std::cout << "Row " << row << ", Age: Error - " << e.what() << "\n";
                }
            }
        } catch (const std::invalid_argument& e) {
            std::cout << "Column '" << column_name << "' not found.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in example2: " << e.what() << "\n";
    }
}

// Example 3: Using Range-Based Loops
// This example uses modern C++ range-based loops for a cleaner iteration over rows and their cells.
void example3() {
    try {
        m2::CSVTable table = create_example_table();
        size_t row_index = 0;
        for (const auto& row : table) {
            std::cout << "Row " << row_index++ << ":\n";
            std::cout << row << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in example3: " << e.what() << "\n";
    }
}

// Example 4: Iterating and Modifying Values
// This example iterates over the table, modifies values in a specific column (e.g., "Score"), and saves the result.
void example4() {
    try {
        m2::CSVTable table = create_example_table();
        for (size_t row = 0; row < table.num_rows(); ++row) {
            try {
                // Increment the "Score" value by 1.0
                double current_score = table.get<double>(row, "Score");
                table[row]["Score"] = current_score + 1.0;
                std::cout << "Updated Score in row " << row << " to " << (current_score + 1.0) << "\n";
            } catch (const std::exception& e) {
                std::cout << "Error updating row " << row << ": " << e.what() << "\n";
            }
        }
        table.save_to_file("_example_.csv"); // Save modified table
    } catch (const std::exception& e) {
        std::cerr << "Error in example4: " << e.what() << "\n";
    }
}

void example5()
{
    m2::CSVTable table = create_example_table();
    std::cout << "Original Table:\n"
              << table << "\n";
    try
    {
        for (auto row : table)
        {
            row["Score"] = row.get<double>("Score") + 1.0; // Increment Score by 1.0
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in example5: " << e.what() << "\n";
    }
    std::cout << "Modified Table:\n"
              << table << "\n";
}

// Main function to run the examples
int main() {
    std::cout << "Running example1()...\n";
    example1();
    std::cout << "Running example2()...\n";
    example2();
    std::cout << "Running example3()...\n";
    example3();
    std::cout << "Running example4()...\n";
    example4();
    std::cout << "Running example5()...\n";
    example5();
    return 0;
}