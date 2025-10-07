#include "CSVTable.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace m2;

int main() {
    std::cout << "=== CSVTable Filter Performance Test ===\n" << std::endl;

    // Create a large test table
    CSVTable table;
    table.add_column<int>("id");
    table.add_column<double>("value");
    table.add_column<std::string>("category");

    const size_t num_rows = 1000000;  // 1 million rows
    std::cout << "Creating test table with " << num_rows << " rows..." << std::endl;

    for (size_t i = 0; i < num_rows; ++i) {
        std::string category = (i % 3 == 0) ? "A" : (i % 3 == 1) ? "B" : "C";
        table.append_row({static_cast<int>(i), static_cast<double>(i) * 1.5, category});
    }

    std::cout << "Table created with " << table.num_rows() << " rows\n" << std::endl;

    // Define a filter predicate (keep rows where value > 500000)
    auto predicate = [](int row_idx, const CSVTable& t) {
        return t.get<double>(row_idx, "value") > 750000.0;
    };

    // Test 1: Original filter_table (no progress)
    std::cout << "Test 1: filter_table() - Original method" << std::endl;
    auto start1 = std::chrono::steady_clock::now();

    auto filtered1 = table.filter_table(predicate);

    auto end1 = std::chrono::steady_clock::now();
    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();

    std::cout << "  Result: " << filtered1.num_rows() << " rows matched" << std::endl;
    std::cout << "  Time: " << ms1 << "ms" << std::endl;
    std::cout << "  Speed: " << std::fixed << std::setprecision(0)
              << (num_rows * 1000.0 / ms1) << " rows/sec\n" << std::endl;

    // Test 2: Optimized filter_table_fast (with progress)
    std::cout << "Test 2: filter_table_fast() - Optimized with progress" << std::endl;
    auto start2 = std::chrono::steady_clock::now();

    auto filtered2 = table.filter_table_fast(predicate, true);

    auto end2 = std::chrono::steady_clock::now();
    auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();

    std::cout << "  Result: " << filtered2.num_rows() << " rows matched" << std::endl;
    std::cout << "  Time: " << ms2 << "ms" << std::endl;
    std::cout << "  Speed: " << std::fixed << std::setprecision(0)
              << (num_rows * 1000.0 / ms2) << " rows/sec\n" << std::endl;

    // Test 3: Optimized filter_table_fast (no progress)
    std::cout << "Test 3: filter_table_fast(false) - Optimized without progress" << std::endl;
    auto start3 = std::chrono::steady_clock::now();

    auto filtered3 = table.filter_table_fast(predicate, false);

    auto end3 = std::chrono::steady_clock::now();
    auto ms3 = std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3).count();

    std::cout << "  Result: " << filtered3.num_rows() << " rows matched" << std::endl;
    std::cout << "  Time: " << ms3 << "ms" << std::endl;
    std::cout << "  Speed: " << std::fixed << std::setprecision(0)
              << (num_rows * 1000.0 / ms3) << " rows/sec\n" << std::endl;

    // Summary
    std::cout << "=== Summary ===" << std::endl;
    std::cout << "Dataset: " << num_rows << " rows, " << filtered1.num_rows()
              << " rows match predicate (" << std::setprecision(1)
              << (filtered1.num_rows() * 100.0 / num_rows) << "%)" << std::endl;
    std::cout << "\nPerformance:" << std::endl;
    std::cout << "  filter_table():           " << ms1 << "ms (baseline)" << std::endl;
    std::cout << "  filter_table_fast(true):  " << ms2 << "ms ("
              << std::setprecision(2) << ((double)ms1/ms2) << "x)" << std::endl;
    std::cout << "  filter_table_fast(false): " << ms3 << "ms ("
              << std::setprecision(2) << ((double)ms1/ms3) << "x)" << std::endl;

    std::cout << "\nKey Optimizations:" << std::endl;
    std::cout << "  ✓ Memory pre-allocation (reserve)" << std::endl;
    std::cout << "  ✓ Direct row copying (no index indirection)" << std::endl;
    std::cout << "  ✓ Optional progress reporting" << std::endl;

    return 0;
}
