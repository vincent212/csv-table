#include "CSVTable.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

namespace m2 {

// Helper to create a test CSV file
void create_test_csv(const std::string& filename, const std::vector<std::string>& lines) {
    std::ofstream file(filename);
    for (const auto& line : lines) {
        file << line << "\n";
    }
}

} // namespace m2

int main() {
    using namespace m2;
    try {
        // Setup: Create test CSV files
        create_test_csv("input.csv", {
            R"("Name","Age","Score")",
            R"("Alice","25","90.5")",
            R"("Bob","30","85.0")",
            R"("Charlie","","95.0")",
            R"("Alice","25","88.0")"
        });

        create_test_csv("input2.csv", {
            R"("Name","Age","City")",
            R"("Alice","25","New York")",
            R"("Bob","35","London")",
            R"("David","40","Paris")"
        });

        // 1. File I/O: Read CSV
        std::cout << "1. Reading CSV file:\n";
        CSVTable table("input.csv");
        std::cout << table << "\n";

        // 2. File I/O: Write CSV
        std::cout << "2. Writing to CSV file:\n";
        table.save_to_file("output.csv");
        std::ifstream output("output.csv");
        std::string line;
        while (std::getline(output, line)) {
            std::cout << line << "\n";
        }
        std::cout << "\n";

        // 3. Data Access: Get values
        std::cout << "3. Accessing values:\n";
        std::cout << "Name at row 0: " << table.get<std::string>(0, "Name") << "\n";
        std::cout << "Age at row 1: " << table.get<int>(1, "Age") << "\n";
        std::cout << "Score at row 2: " << table.get<double>(2, "Score") << "\n\n";

        // 4. Data Modification: Modify values
        std::cout << "4. Modifying values:\n";
        table[0]["Score"] = 91.0;
        std::cout << "Updated Score at row 0: " << table.get<double>(0, "Score") << "\n\n";

        // 5. Data Modification: Set column type
        std::cout << "5. Setting column type:\n";
        table.set_column_type<int>("Age", true, 0);
        std::cout << "Age at row 2 after conversion: " << table.get<int>(2, "Age") << "\n\n";

        // 6. Column Operation: Add column
        std::cout << "6. Adding column:\n";
        table.add_column("Bonus", 100.0);
        std::cout << "Bonus at row 0: " << table.get<double>(0, "Bonus") << "\n\n";

        // 7. Column Operation: Delete column
        std::cout << "7. Deleting column:\n";
        table.delete_column("Bonus");
        std::cout << "Columns after deletion: ";
        for (const auto& col : table.get_rows()[0]) {
            std::cout << CSVTable::cell_to_string(col) << " ";
        }
        std::cout << "\n\n";

        // 8. Column Operation: Rename columns
        std::cout << "8. Renaming columns:\n";
        table.rename_columns({{"Score", "Points"}});
        std::cout << "Value in Points at row 0: " << table.get<double>(0, "Points") << "\n\n";

        // 9. Row Operation: Append row
        std::cout << "9. Appending row:\n";
        table.append_row({std::string("David"), 40, 88.0});
        std::cout << "New row Name: " << table.get<std::string>(table.get_rows().size() - 1, "Name") << "\n\n";

        // 10. Row Operation: Filter rows
        std::cout << "10. Filtering rows:\n";
        auto indices = table.filter_rows([](int row, const CSVTable& t) {
            return t.get<int>(row, "Age") > 25;
        });
        std::cout << "Filtered row indices: ";
        for (int idx : indices) {
            std::cout << idx << " ";
        }
        std::cout << "\n\n";

        // 11. Row Operation: Filter table
        std::cout << "11. Creating filtered table:\n";
        auto filtered = table.filter_table([](int row, const CSVTable& t) {
            return t.get<int>(row, "Age") > 25;
        });
        std::cout << filtered << "\n";

        // 12. Row Operation: Sub-table
        std::cout << "12. Creating sub-table:\n";
        auto sub = table.sub_table({0, 2});
        std::cout << sub << "\n";

        // 13. Row Operation: Modify rows
        std::cout << "13. Modifying rows:\n";
        table.modify([](int row, CSVTable& t) {
            t[row]["Points"] = t.get<double>(row, "Points") + 1.0;
        });
        std::cout << "Updated Points at row 0: " << table.get<double>(0, "Points") << "\n\n";

        // 14. Row Operation: Drop NA
        std::cout << "14. Dropping rows with NA:\n";
        table.dropna({"Age"});
        std::cout << "Rows after dropna:\n" << table << "\n";

        // 15. Row Operation: Fill NA
        std::cout << "15. Filling NA values:\n";
        CSVTable table_with_na("input.csv");
        table_with_na.fillna({"Age"}, 0);
        std::cout << "Age at row 2 after fillna: " << table_with_na.get<int>(2, "Age") << "\n\n";

        // 16. Row Operation: Drop duplicates
        std::cout << "16. Dropping duplicates:\n";
        table.drop_duplicates({"Name", "Age"});
        std::cout << "Rows after dropping duplicates:\n" << table << "\n";

        // 17. Sorting: Sort by column
        std::cout << "17. Sorting by column:\n";
        table.sort_by_column<int>("Age", true);
        std::cout << "Table after sorting by Age:\n" << table << "\n";

        // 18. Merging: merge
        std::cout << "18. Standard merge (inner):\n";
        CSVTable table2("input2.csv");
        auto merged = table.merge(table2, {"Name", "Age"}, "inner");
        std::cout << merged << "\n";

        // 19. & 20. deleted

        // 21. Joining: Join
        std::cout << "21. Join (left):\n";
        auto joined = table.join(table2, "left");
        std::cout << joined << "\n";

        // 22. Utility: Error handling example
        std::cout << "22. Error handling example:\n";
        try {
            table.get<std::string>(10, "Name");
        } catch (const std::out_of_range& e) {
            std::cout << "Caught expected error: " << e.what() << "\n\n";
        }

        // 23. Utility: Streaming
        std::cout << "23. Streaming table:\n";
        std::cout << table << "\n";

        // Cleanup
        std::filesystem::remove("input.csv");
        std::filesystem::remove("input2.csv");
        std::filesystem::remove("output.csv");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}