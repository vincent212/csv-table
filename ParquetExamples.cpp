#include "CSVTable.hpp"
#include <iostream>
#include <vector>

using namespace m2;

int main() {
    try {
        std::cout << "=== CSVTable Parquet Examples ===\n\n";

        // Example 1: Create a table and save to Parquet
        std::cout << "1. Creating a table and saving to Parquet...\n";
        CSVTable table1;
        table1.add_column<std::string>("Name");
        table1.add_column<int>("Age");
        table1.add_column<double>("Score");
        table1.add_column<bool>("Active");
        table1.add_column<uint64_t>("ID");

        table1.append_row({std::string("Alice"), 30, 95.5, true, uint64_t(1001)});
        table1.append_row({std::string("Bob"), 25, 87.3, false, uint64_t(1002)});
        table1.append_row({std::string("Charlie"), 35, 92.1, true, uint64_t(1003)});
        table1.append_row({std::string("Diana"), 28, 98.7, true, uint64_t(1004)});

        std::cout << "Original table:\n" << table1 << "\n";

        // Save to Parquet
        table1.save_to_parquet("test_data.parquet");
        std::cout << "Saved to test_data.parquet\n\n";

        // Example 2: Read from Parquet
        std::cout << "2. Reading from Parquet file...\n";
        CSVTable table2;
        table2.read_parquet("test_data.parquet");
        std::cout << "Table loaded from Parquet:\n" << table2 << "\n";

        // Example 3: Convert CSV to Parquet
        std::cout << "3. Converting CSV to Parquet...\n";

        // First create a sample CSV file
        CSVTable csv_table;
        csv_table.add_column<std::string>("Product");
        csv_table.add_column<int>("Quantity");
        csv_table.add_column<double>("Price");

        csv_table.append_row({std::string("Apple"), 100, 1.50});
        csv_table.append_row({std::string("Banana"), 150, 0.75});
        csv_table.append_row({std::string("Orange"), 80, 1.25});

        csv_table.save_to_file("products.csv");
        std::cout << "Created products.csv\n";

        // Convert to Parquet
        csv_table.save_to_parquet("products.parquet");
        std::cout << "Converted to products.parquet\n\n";

        // Example 4: Read Parquet and manipulate
        std::cout << "4. Reading Parquet and performing operations...\n";
        CSVTable table4;
        table4.read_parquet("test_data.parquet");

        // Filter rows where Active is true
        auto filtered = table4.filter_table([](int row_idx, const CSVTable& t) {
            return t.get<bool>(row_idx, "Active");
        });

        std::cout << "Filtered table (Active = true):\n" << filtered << "\n";

        // Sort by Score
        filtered.sort_by_column<double>("Score", false);  // descending
        std::cout << "Sorted by Score (descending):\n" << filtered << "\n";

        // Save filtered result
        filtered.save_to_parquet("filtered_data.parquet");
        std::cout << "Saved filtered data to filtered_data.parquet\n\n";

        // Example 5: Performance comparison
        std::cout << "5. Creating larger dataset for format comparison...\n";
        CSVTable large_table;
        large_table.add_column<int>("Index");
        large_table.add_column<double>("Value1");
        large_table.add_column<double>("Value2");
        large_table.add_column<std::string>("Category");

        for (int i = 0; i < 10000; ++i) {
            std::string category = (i % 3 == 0) ? "A" : (i % 3 == 1) ? "B" : "C";
            large_table.append_row({i, i * 1.5, i * 2.3, category});
        }

        std::cout << "Created table with " << large_table.num_rows() << " rows\n";

        // Save as CSV
        large_table.save_to_file("large_data.csv");
        std::cout << "Saved as CSV: large_data.csv\n";

        // Save as Parquet
        large_table.save_to_parquet("large_data.parquet");
        std::cout << "Saved as Parquet: large_data.parquet\n";

        // Compare file sizes
        std::cout << "\nFile size comparison:\n";
        std::filesystem::path csv_path("large_data.csv");
        std::filesystem::path parquet_path("large_data.parquet");

        if (std::filesystem::exists(csv_path)) {
            std::cout << "  CSV:     " << std::filesystem::file_size(csv_path) << " bytes\n";
        }
        if (std::filesystem::exists(parquet_path)) {
            std::cout << "  Parquet: " << std::filesystem::file_size(parquet_path) << " bytes\n";
        }

        // Example 6: Type preservation
        std::cout << "\n6. Testing type preservation in Parquet...\n";
        CSVTable type_test;
        type_test.add_column<uint64_t>("BigNumber");
        type_test.add_column<bool>("Flag");
        type_test.add_column<int>("SmallNumber");

        type_test.append_row({uint64_t(9999999999999), true, -42});
        type_test.append_row({uint64_t(1234567890123), false, 100});

        std::cout << "Original:\n" << type_test << "\n";

        type_test.save_to_parquet("types.parquet");

        CSVTable type_loaded;
        type_loaded.read_parquet("types.parquet");
        std::cout << "After Parquet round-trip:\n" << type_loaded << "\n";

        std::cout << "=== All examples completed successfully! ===\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
