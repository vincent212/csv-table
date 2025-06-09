#include <gtest/gtest.h>
#include "CSVTable.hpp"
#include <fstream>
#include <vector>
#include <filesystem>

namespace m2 {

    // Helper function to write CSV content to a file
void write_csv(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filename);
    }
    file << content;
    file.close();
}

class CSVTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary CSV file for testing
        create_test_csv("test.csv", {
            "Name,Age,Score,ID",
            "Alice,25,90.5,123456789012345",
            "Bob,30,85.0,987654321098765",
            "Charlie,,95.0,555555555555555"
        });

        // Create a second CSV file for merge/join tests
        create_test_csv("test2.csv", {
            "Name,Age,City",
            "Alice,25,New York",
            "Bob,35,London",
            "David,40,Paris"
        });
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove("test.csv");
        std::filesystem::remove("test2.csv");
        std::filesystem::remove("output.csv");
        std::filesystem::remove("test_type.csv");
        std::filesystem::remove("test_dups.csv");
    }

    // Helper to create a temporary CSV file
    void create_test_csv(const std::string& filename, const std::vector<std::string>& lines) {
        std::ofstream file(filename);
        for (const auto& line : lines) {
            file << line << "\n";
        }
    }
};

// Test 1: Constructor and file reading
TEST_F(CSVTableTest, ConstructorAndFileReading) {
    CSVTable table("test.csv");
    EXPECT_EQ(table.get_rows().size(), 3) << "Constructor: Row count";
    EXPECT_EQ(table.get_rows()[0].size(), 4) << "Constructor: Column count";
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice") << "Constructor: Name value";
    EXPECT_EQ(table.get<int>(0, "Age"), 25) << "Constructor: Age value";
    EXPECT_DOUBLE_EQ(table.get<double>(0, "Score"), 90.5) << "Constructor: Score value";
    EXPECT_EQ(table.get<uint64_t>(0, "ID"), 123456789012345ULL) << "Constructor: ID value";

    // Test failure: Invalid file
    EXPECT_THROW(CSVTable("nonexistent.csv"), std::runtime_error) << "Constructor: Should throw on nonexistent file";
}

// Test 2: Set column type
TEST_F(CSVTableTest, SetColumnType) {
    create_test_csv("test_type.csv", {
        "Name,Age,Score,ID",
        "Alice,25,90.5,123456789012345",
        "Bob,invalid,85.0,987654321098765"
    });
    CSVTable table("test_type.csv");

    EXPECT_NO_THROW(table.set_column_type<int>("Age", true, 0)) << "Set column type: Handle invalid with skip_errors";
    EXPECT_EQ(table.get<int>(0, "Age"), 25) << "Set column type: Valid conversion";
    EXPECT_EQ(table.get<int>(1, "Age"), 0) << "Set column type: Invalid replaced with default";

    // Test uint64_t conversion
    EXPECT_NO_THROW(table.set_column_type<uint64_t>("ID")) << "Set column type: uint64_t conversion";
    EXPECT_EQ(table.get<uint64_t>(0, "ID"), 123456789012345ULL) << "Set column type: uint64_t value";

    // Test failure: Invalid column
    EXPECT_THROW(table.set_column_type<int>("Invalid"), std::invalid_argument) << "Set column type: Invalid column";

    // Test failure: Strict conversion
    EXPECT_THROW(table.set_column_type<int>("Name"), std::runtime_error) << "Set column type: Should throw on invalid value";
}

// Test 3: Get value
TEST_F(CSVTableTest, GetValue) {
    CSVTable table("test.csv");
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice") << "Get: String value";
    EXPECT_EQ(table.get<int>(0, "Age"), 25) << "Get: Int value";
    EXPECT_DOUBLE_EQ(table.get<double>(0, "Score"), 90.5) << "Get: Double value";
    EXPECT_EQ(table.get<uint64_t>(0, "ID"), 123456789012345ULL) << "Get: uint64_t value";

    // Test failure: Invalid row
    EXPECT_THROW(table.get<std::string>(10, "Name"), std::out_of_range) << "Get: Invalid row";

    // Test failure: Invalid column
    EXPECT_THROW(table.get<std::string>(0, "Invalid"), std::invalid_argument) << "Get: Invalid column";

    // Test failure: Type mismatch
    EXPECT_THROW(table.get<bool>(0, "Name"), std::runtime_error) << "Get: Type mismatch";
}

TEST_F(CSVTableTest, AddColumn) {
    CSVTable table("test.csv");
    table.add_column("NewID", uint64_t(1000));
    EXPECT_EQ(table.get_rows()[0].size(), 5) << "Add column: Column count";
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice") << "Add column: Name still accessible";
    EXPECT_EQ(table.get<int>(0, "Age"), 25) << "Add column: Age still accessible";
    EXPECT_EQ(table.get<double>(0, "Score"), 90.5) << "Add column: Score still accessible";
    EXPECT_EQ(table.get<uint64_t>(0, "ID"), 123456789012345ULL) << "Add column: ID still accessible";
    EXPECT_EQ(table.get<uint64_t>(0, "NewID"), 1000ULL) << "Add column: Default value";

    // Test failure: Duplicate column
    EXPECT_THROW(table.add_column("NewID", uint64_t(2000)), std::invalid_argument) << "Add column: Duplicate column";
}

TEST_F(CSVTableTest, DeleteColumn) {
    CSVTable table("test.csv");
    table.delete_column("Age");
    EXPECT_EQ(table.get_rows()[0].size(), 3) << "Delete column: Column count";
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice") << "Delete column: Name still accessible";
    EXPECT_EQ(table.get<double>(0, "Score"), 90.5) << "Delete column: Score still accessible";
    EXPECT_EQ(table.get<uint64_t>(0, "ID"), 123456789012345ULL) << "Delete column: ID still accessible";
    EXPECT_THROW(table.get<int>(0, "Age"), std::invalid_argument) << "Delete column: Age no longer accessible";

    // Test failure: Invalid column
    EXPECT_THROW(table.delete_column("Invalid"), std::invalid_argument) << "Delete column: Invalid column";
}

// Test 6: Append row
TEST_F(CSVTableTest, AppendRow) {
    CSVTable table("test.csv");
    table.append_row({std::string("David"), 40, 88.0, uint64_t(111111111111111)});
    EXPECT_EQ(table.get_rows().size(), 4) << "Append row: Row count";
    EXPECT_EQ(table.get<std::string>(3, "Name"), "David") << "Append row: Name value";
    EXPECT_EQ(table.get<int>(3, "Age"), 40) << "Append row: Age value";
    EXPECT_DOUBLE_EQ(table.get<double>(3, "Score"), 88.0) << "Append row: Score value";
    EXPECT_EQ(table.get<uint64_t>(3, "ID"), 111111111111111ULL) << "Append row: ID value";
}

// Test 7: Filter rows
TEST_F(CSVTableTest, FilterRows) {
    CSVTable table("test.csv");
    auto indices = table.filter_rows([](int row, const CSVTable& t) {
        try {
            return t.get<int>(row, "Age") > 25;
        } catch (...) {
            return false;
        }
    });
    EXPECT_EQ(indices.size(), 1) << "Filter rows: Correct count";
    EXPECT_EQ(indices[0], 1) << "Filter rows: Correct index";
}

// Test 8: Filter table
TEST_F(CSVTableTest, FilterTable) {
    CSVTable table("test.csv");
    auto filtered = table.filter_table([](int row, const CSVTable& t) {
        try {
            return t.get<int>(row, "Age") > 25;
        } catch (...) {
            return false;
        }
    });
    EXPECT_EQ(filtered.get_rows().size(), 1) << "Filter table: Row count";
    EXPECT_EQ(filtered.get<std::string>(0, "Name"), "Bob") << "Filter table: Name value";
}

// Test 9: Sub-table
TEST_F(CSVTableTest, SubTable) {
    CSVTable table("test.csv");
    auto sub = table.sub_table({0, 2});
    EXPECT_EQ(sub.get_rows().size(), 2) << "Sub-table: Row count";
    EXPECT_EQ(sub.get<std::string>(0, "Name"), "Alice") << "Sub-table: First row";
    EXPECT_EQ(sub.get<std::string>(1, "Name"), "Charlie") << "Sub-table: Second row";

    // Test failure: Invalid index
    EXPECT_THROW(table.sub_table({10}), std::out_of_range) << "Sub-table: Invalid index";
}

// Test 10: Modify rows
TEST_F(CSVTableTest, ModifyRows) {
    CSVTable table("test.csv");
    table.modify([](int row, CSVTable& t) {
        try {
            t[row]["Score"] = t.get<double>(row, "Score") + 1.0;
        } catch (...) {
            t[row]["Score"] = 0.0;
        }
    });
    EXPECT_DOUBLE_EQ(table.get<double>(0, "Score"), 91.5) << "Modify: Updated score";
    EXPECT_DOUBLE_EQ(table.get<double>(2, "Score"), 96.0) << "Modify: Updated score";
}

// Test 11: Apply to column
TEST_F(CSVTableTest, ApplyToColumn) {
    CSVTable table("test.csv");

    // Test 1: Increment Age (int)
    table.apply_to_column<int>("Age", [](const CSVTable::CellValue& value) -> int {
        if (std::holds_alternative<std::string>(value) && std::get<std::string>(value).empty()) {
            return 0;
        }
        return std::visit([](const auto& v) -> int {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return std::stoi(v);
            } else {
                return static_cast<int>(v);
            }
        }, value) + 1;
    });
    EXPECT_EQ(table.get<int>(0, "Age"), 26) << "Apply to column: Age incremented";
    EXPECT_EQ(table.get<int>(2, "Age"), 0) << "Apply to column: Missing Age set to 0";

    // Test 2: Add 1000 to ID (uint64_t)
    table.set_column_type<uint64_t>("ID");
    table.apply_to_column<uint64_t>("ID", [](const CSVTable::CellValue& value) -> uint64_t {
        return std::visit([](const auto& v) -> uint64_t {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return std::stoull(v);
            } else {
                return static_cast<uint64_t>(v);
            }
        }, value) + 1000;
    });
    EXPECT_EQ(table.get<uint64_t>(0, "ID"), 123456789013345ULL) << "Apply to column: ID incremented";

    // Test 3: Append text to Name (string)
    table.apply_to_column<std::string>("Name", [](const CSVTable::CellValue& value) -> std::string {
        return std::visit([](const auto& v) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return v + "_Test";
            } else {
                return std::to_string(v) + "_Test";
            }
        }, value);
    });
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice_Test") << "Apply to column: Name appended";

    // Test failure: Invalid column
    EXPECT_THROW(
        table.apply_to_column<int>("Invalid", [](const CSVTable::CellValue&) { return 0; }),
        std::invalid_argument
    ) << "Apply to column: Invalid column";
}

// Test 12: Drop NA
TEST_F(CSVTableTest, DropNA) {
    CSVTable table("test.csv");
    table.dropna({"Age"});
    EXPECT_EQ(table.get_rows().size(), 2) << "Drop NA: Row count";
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice") << "Drop NA: First row";
    EXPECT_EQ(table.get<std::string>(1, "Name"), "Bob") << "Drop NA: Second row";

    // Test failure: Invalid column
    EXPECT_THROW(table.dropna({"Invalid"}), std::invalid_argument) << "Drop NA: Invalid column";
}

// Test 13: Rename columns
TEST_F(CSVTableTest, RenameColumns) {
    CSVTable table("test.csv");
    table.rename_columns({{"Age", "Years"}});
    EXPECT_EQ(table.get<int>(0, "Years"), 25) << "Rename columns: Renamed column access";
    EXPECT_THROW(table.get<int>(0, "Age"), std::invalid_argument) << "Rename columns: Old name inaccessible";

    // Test failure: Invalid old column
    EXPECT_THROW(table.rename_columns({{"Invalid", "New"}}), std::invalid_argument) << "Rename columns: Invalid old column";

    // Test failure: Duplicate new column
    table.rename_columns({{"Name", "NewName"}});
    EXPECT_THROW(table.rename_columns({{"Score", "NewName"}}), std::invalid_argument) << "Rename columns: Duplicate new column";
}

// Test 14: Sort by column
TEST_F(CSVTableTest, SortByColumn) {
    CSVTable table("test.csv");
    table.set_column_type<int>("Age", true, 0);
    table.sort_by_column<int>("Age", true);
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Charlie") << "Sort: Ascending order";
    EXPECT_EQ(table.get<std::string>(1, "Name"), "Alice") << "Sort: Ascending order";
    table.sort_by_column<int>("Age", false);
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Bob") << "Sort: Descending order";

    // Test uint64_t sorting
    table.set_column_type<uint64_t>("ID");
    table.sort_by_column<uint64_t>("ID", true);
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice") << "Sort: Ascending order by ID";

    // Test failure: Invalid column
    EXPECT_THROW(table.sort_by_column<int>("Invalid", true), std::invalid_argument) << "Sort: Invalid column";

    // Test failure: Type mismatch
    EXPECT_THROW(table.sort_by_column<bool>("Name", true), std::runtime_error) << "Sort: Type mismatch";
}

// Test 15: Fill NA
TEST_F(CSVTableTest, FillNA) {
    CSVTable table("test.csv");
    table.fillna({"Age"}, 0);
    EXPECT_EQ(table.get<int>(2, "Age"), 0) << "Fill NA: Replaced empty with 0";

    // Test failure: Invalid column
    EXPECT_THROW(table.fillna({"Invalid"}, 0), std::invalid_argument) << "Fill NA: Invalid column";
}

// Test 16: Drop duplicates
TEST_F(CSVTableTest, DropDuplicates) {
    create_test_csv("test_dups.csv", {
        R"("Name","Age","ID")",
        R"("Alice","25","123456789012345")",
        R"("Alice","25","123456789012345")",
        R"("Bob","30","987654321098765")"
    });
    CSVTable table("test_dups.csv");
    table.drop_duplicates({"Name", "Age"});
    EXPECT_EQ(table.get_rows().size(), 2) << "Drop duplicates: Row count";
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice") << "Drop duplicates: First row";
    EXPECT_EQ(table.get<std::string>(1, "Name"), "Bob") << "Drop duplicates: Second row";

    // Test failure: Invalid column
    EXPECT_THROW(table.drop_duplicates({"Invalid"}), std::invalid_argument) << "Drop duplicates: Invalid column";
}

// Test 17: Merge
TEST_F(CSVTableTest, Merge) {
    CSVTable table1("test.csv");
    CSVTable table2("test2.csv");
    auto merged = table1.merge(table2, {"Name", "Age"}, "inner");
    EXPECT_EQ(merged.get_rows().size(), 1) << "Merge: Inner join row count";
    EXPECT_EQ(merged.get<std::string>(0, "Name"), "Alice") << "Merge: Inner join Name";
    EXPECT_EQ(merged.get<std::string>(0, "City"), "New York") << "Merge: Inner join City";

    auto left = table1.merge(table2, {"Name", "Age"}, "left");
    EXPECT_EQ(left.get_rows().size(), 3) << "Merge: Left join row count";

    // Test failure: Invalid column
    EXPECT_THROW(table1.merge(table2, {"Invalid"}, "inner"), std::invalid_argument) << "Merge: Invalid column";

    // Test failure: Invalid join type
    EXPECT_THROW(table1.merge(table2, {"Name"}, "invalid"), std::invalid_argument) << "Merge: Invalid join type";
}

// Test 18 deleted

// Test 19: deleted

// Test 20: Join
TEST_F(CSVTableTest, Join) {
    CSVTable table1("test.csv");
    CSVTable table2("test2.csv");
    auto joined = table1.join(table2, "left");
    EXPECT_EQ(joined.get_rows().size(), 3) << "Join: Left join row count";
    EXPECT_EQ(joined.get<std::string>(0, "Name"), "Alice") << "Join: Name";
    EXPECT_EQ(joined.get<std::string>(0, "City"), "New York") << "Join: City";

    auto outer = table1.join(table2, "outer");
    EXPECT_EQ(outer.get_rows().size(), 3) << "Join: Outer join row count";

    // Test failure: Invalid join type
    EXPECT_THROW(table1.join(table2, "invalid"), std::invalid_argument) << "Join: Invalid join type";
}

// Test 21: Save to file
TEST_F(CSVTableTest, SaveToFile) {
    CSVTable table("test.csv");
    table.save_to_file("output.csv");
    std::ifstream file("output.csv");
    std::string line;
    std::getline(file, line);
    EXPECT_EQ(line, "Name,Age,Score,ID") << "Save to file: Header";
    std::getline(file, line);
    EXPECT_EQ(line, "Alice,25,90.5000000000,123456789012345") << "Save to file: First row";

    // Test failure: Invalid file path
    EXPECT_THROW(table.save_to_file("/invalid/path/output.csv"), std::runtime_error) << "Save to file: Invalid path";
}

// Test 22: Stream output
TEST_F(CSVTableTest, StreamOutput) {
    CSVTable table("test.csv");
    std::ostringstream oss;
    oss << table;
    std::string output = oss.str();
    std::string expected = "Name,Age,Score,ID\nAlice,25,90.5000000000,123456789012345\nBob,30,85,987654321098765\nCharlie,,95,555555555555555\n";
    EXPECT_EQ(output, expected) << "Stream output: Correct format";
}


TEST_F(CSVTableTest, AppendFiles) {
    // Create temporary CSV files
    std::string file1 = "test1.csv";
    std::string file2 = "test2.csv";
    std::string file3 = "test3.csv";

    // Write sample data to file1 and file2 with same columns
    write_csv(file1, "Name,Age\nAlice,30\nBob,25\n");
    write_csv(file2, "Name,Age\nCharlie,35\nDavid,28\n");

    // Write sample data to file3 with different columns
    write_csv(file3, "Name,Salary\nEve,50000\nFrank,60000\n");

    // Test reading first file
    m2::CSVTable table;
    table.read_file(file1);
    EXPECT_EQ(table.get_rows().size(), 2);
    EXPECT_EQ(table.get_col_names(), std::vector<std::string>({"Name", "Age"}));

    // Test appending second file
    table.read_file(file2);
    EXPECT_EQ(table.get_rows().size(), 4);
    // Check specific values
    EXPECT_EQ(table.get<std::string>(0, "Name"), "Alice");
    EXPECT_EQ(table.get<std::string>(1, "Name"), "Bob");
    EXPECT_EQ(table.get<std::string>(2, "Name"), "Charlie");
    EXPECT_EQ(table.get<std::string>(3, "Name"), "David");

    // Test appending file with different columns
    EXPECT_THROW(table.read_file(file3), std::runtime_error);

    // Clean up temporary files
    std::filesystem::remove(file1);
    std::filesystem::remove(file2);
    std::filesystem::remove(file3);
}

// Helper function to add a row to the table
void add_row(CSVTable& table, const std::vector<CSVTable::CellValue>& values) {
    table.append_row(values);
}

// Test case for the find method
TEST_F(CSVTableTest, LowerBound) {
    CSVTable table;
    // Initialize table with columns: "name" (string), "age" (int)
    table.add_column<std::string>("name", "");
    table.add_column<int>("age", 0);

    // Add rows
    add_row(table, {"Alice", 25});
    add_row(table, {"Bob", 30});
    add_row(table, {"Charlie", 35});
    add_row(table, {"David", 40});

    // Sort the table by "age" in ascending order
    table.sort_by_column<int>("age", true);

    // Create a const reference to table to get ConstRowIterator from end()
    const auto& const_table = table;

    // Test 1: Exact match found
    auto it1 = table.lower_bound<int>("age", 30);
    ASSERT_NE(it1, const_table.end()) << "Iterator should not be at end for exact match";
    const auto& row1 = *it1;
    EXPECT_EQ(row1.get<std::string>("name"), "Bob") << "Name should be Bob";
    EXPECT_EQ(row1.get<int>("age"), 30) << "Age should be 30";

    // Test 2: No exact match, find first greater
    auto it2 = table.lower_bound<int>("age", 32);
    ASSERT_NE(it2, const_table.end()) << "Iterator should point to first greater value";
    const auto& row2 = *it2;
    EXPECT_EQ(row2.get<std::string>("name"), "Charlie") << "Name should be Charlie";
    EXPECT_EQ(row2.get<int>("age"), 35) << "Age should be 35";

    // Test 3: All values less than the search value
    auto it3 = table.lower_bound<int>("age", 45);
    EXPECT_EQ(it3, const_table.end()) << "Iterator should be at end when value exceeds all";

    // Test 4: Column not found
    EXPECT_THROW({
        table.lower_bound<int>("height", 30);
    }, std::invalid_argument) << "Should throw invalid_argument for unknown column";

    // Test 5: Conversion failure
    EXPECT_THROW({
        table.lower_bound<int>("name", 30);
    }, std::invalid_argument) << "Should throw invalid_argument for type mismatch";
}

// Test finding an existing integer value
TEST_F(CSVTableTest, FindIntegerExisting) {
    CSVTable table;
    table.add_column<int>("id");
    table.add_column<std::string>("name");
    // Add rows in sorted order by "id"
    add_row(table, {1, "Alice"});
    add_row(table, {2, "Bob"});
    add_row(table, {3, "Charlie"});
    add_row(table, {4, "David"});
    const auto& const_table = table;
    auto it = table.find<int>("id", 3);
    ASSERT_NE(it, const_table.end()) << "Iterator should not be at end for exact match";
    const auto& row = *it;
    EXPECT_EQ(row.get<int>("id"), 3);
    EXPECT_EQ(row.get<std::string>("name"), "Charlie");
}

// Test finding a non-existing integer value
TEST_F(CSVTableTest, FindIntegerNonExisting) {
    CSVTable table;
    table.add_column<int>("id");
    table.add_column<std::string>("name");
    add_row(table, {1, "Alice"});
    add_row(table, {2, "Bob"});
    add_row(table, {3, "Charlie"});
    add_row(table, {4, "David"});
    const auto& const_table = table;
    auto it = table.find<int>("id", 5);
    EXPECT_EQ(it, const_table.end()) << "Iterator should be at end for non-existing value";
}

// Test finding an integer smaller than all values
TEST_F(CSVTableTest, FindIntegerSmallerThanAll) {
    CSVTable table;
    table.add_column<int>("id");
    table.add_column<std::string>("name");
    add_row(table, {1, "Alice"});
    add_row(table, {2, "Bob"});
    add_row(table, {3, "Charlie"});
    add_row(table, {4, "David"});
    const auto& const_table = table;
    auto it = table.find<int>("id", 0);
    EXPECT_EQ(it, const_table.end()) << "Iterator should be at end for value smaller than all";
}

// Test finding an integer larger than all values
TEST_F(CSVTableTest, FindIntegerLargerThanAll) {
    CSVTable table;
    table.add_column<int>("id");
    table.add_column<std::string>("name");
    add_row(table, {1, "Alice"});
    add_row(table, {2, "Bob"});
    add_row(table, {3, "Charlie"});
    add_row(table, {4, "David"});
    const auto& const_table = table;
    auto it = table.find<int>("id", 10);
    EXPECT_EQ(it, const_table.end()) << "Iterator should be at end for value larger than all";
}

// Test finding an existing string value
TEST_F(CSVTableTest, FindStringExisting) {
    CSVTable table;
    table.add_column<std::string>("name");
    table.add_column<int>("age");
    // Add rows in sorted order by "name"
    add_row(table, {"Alice", 25});
    add_row(table, {"Bob", 30});
    add_row(table, {"Charlie", 35});
    add_row(table, {"David", 40});
    const auto& const_table = table;
    auto it = table.find<std::string>("name", "Charlie");
    ASSERT_NE(it, const_table.end()) << "Iterator should not be at end for exact match";
    const auto& row = *it;
    EXPECT_EQ(row.get<std::string>("name"), "Charlie");
    EXPECT_EQ(row.get<int>("age"), 35);
}

// Test finding a non-existing string value
TEST_F(CSVTableTest, FindStringNonExisting) {
    CSVTable table;
    table.add_column<std::string>("name");
    table.add_column<int>("age");
    add_row(table, {"Alice", 25});
    add_row(table, {"Bob", 30});
    add_row(table, {"Charlie", 35});
    add_row(table, {"David", 40});
    const auto& const_table = table;
    auto it = table.find<std::string>("name", "Eve");
    EXPECT_EQ(it, const_table.end()) << "Iterator should be at end for non-existing value";
}

// Test finding in an empty table
TEST_F(CSVTableTest, FindInEmptyTable) {
    CSVTable table;
    table.add_column<int>("id");
    const auto& const_table = table;
    auto it = table.find<int>("id", 1);
    EXPECT_EQ(it, const_table.end()) << "Iterator should be at end for empty table";
}

// Test finding with a non-existent column
TEST_F(CSVTableTest, FindNonExistentColumn) {
    CSVTable table;
    table.add_column<int>("id");
    add_row(table, {1});
    EXPECT_THROW(table.find<int>("nonexistent", 1), std::invalid_argument) << "Should throw invalid_argument for non-existent column";
}


// Test fixture for CSVTable tests
class CSVTableTest2 : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary CSV file for testing
        std::ofstream file("test.csv");
        file << "Name,Age,Score,ID\n";
        file << "Alice,25,90.5,123456789012345\n";
        file << "Bob,30,85.0,987654321098765\n";
        file.close();
    }

    void TearDown() override {
        // Clean up temporary file
        std::filesystem::remove("test.csv");
    }
};

// Test case for assignment and access via Row
TEST_F(CSVTableTest2, RowAssignmentAndAccess) {
    CSVTable table("test.csv");

    // Test 1: Read access via Row
    {
        auto row = table.get_row(0);
        EXPECT_EQ(row.get<std::string>("Name"), "Alice") << "Row read: Name value";
        EXPECT_EQ(row.get<int>("Age"), 25) << "Row read: Age value";
        EXPECT_EQ(row.get<double>("Score"), 90.5) << "Row read: Score value";
        EXPECT_EQ(row.get<uint64_t>("ID"), 123456789012345ULL) << "Row read: ID value";
    }

    // Test 2: Write access via Row
    {
        auto row = table.get_row(0);
        row["Name"] = std::string("Charlie");
        row["Age"] = 35;
        row["Score"] = 95.0;
        row["ID"] = uint64_t(111222333444555ULL);
        // Verify changes in table
        EXPECT_EQ(table.get<std::string>(0, "Name"), "Charlie") << "Row write: Name updated";
        EXPECT_EQ(table.get<int>(0, "Age"), 35) << "Row write: Age updated";
        EXPECT_EQ(table.get<double>(0, "Score"), 95.0) << "Row write: Score updated";
        EXPECT_EQ(table.get<uint64_t>(0, "ID"), 111222333444555ULL) << "Row write: ID updated";
        // Verify other row is unchanged
        EXPECT_EQ(table.get<std::string>(1, "Name"), "Bob") << "Row write: Other row Name unchanged";
    }

    // Test 3: Write access via iterator
    {
        for (auto it = table.begin(); it != table.end(); ++it) {
            auto row = it.row();
            if (it.index() == 1) {
                row["Age"] = 40;
                row["Score"] = 88.5;
            }
        }
        EXPECT_EQ(table.get<int>(1, "Age"), 40) << "Iterator write: Age updated";
        EXPECT_EQ(table.get<double>(1, "Score"), 88.5) << "Iterator write: Score updated";
        EXPECT_EQ(table.get<std::string>(0, "Name"), "Charlie") << "Iterator write: Other row Name unchanged";
    }

    // Test 4: Invalid column access
    {
        auto row = table.get_row(0);
        EXPECT_THROW(row["Invalid"], std::invalid_argument) << "Row access: Invalid column name";
        EXPECT_THROW(row.get<int>("Invalid"), std::invalid_argument) << "Row get: Invalid column name";
    }

    // Test 5: Invalid row index
    {
        EXPECT_THROW(table.get_row(table.num_rows()), std::out_of_range) << "Row access: Out-of-range index";
    }
}


} // namespace m2

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}