#include "CSVTable.hpp"
#include <iostream>
#include <fstream>

namespace m2 {

void create_test_csv(const std::string& filename) {
    std::ofstream file(filename);
    file << "\"Name\",\"ID\",\"Age\"\n";
    file << "\"Alice\",\"123456789012345\",\"25\"\n";
    file << "\"Bob\",\"987654321098765\",\"30\"\n";
    file << "\"Charlie\",\"555555555555555\",\"\"\n";
}

} // namespace m2

int main() {
    using namespace m2;
    try {
        create_test_csv("test.csv");
        CSVTable table("test.csv");
        std::cout << "Original table:\n" << table << "\n";

        table.set_column_type<uint64_t>("ID");
        std::cout << "ID at row 0: " << table.get<uint64_t>(0, "ID") << "\n";

        // Apply transformation to "Age" column
        table.apply_to_column<int>("Age", [](const m2::CSVTable::CellValue& value) -> int {
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
        std::cout << "Age at row 0 after increment: " << table.get<int>(0, "Age") << "\n";
        std::cout << "Age at row 2 after increment: " << table.get<int>(2, "Age") << "\n";

        // Apply transformation to "ID" column
        table.apply_to_column<uint64_t>("ID", [](const m2::CSVTable::CellValue& value) -> uint64_t {
            return std::visit([](const auto& v) -> uint64_t {
                if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                    return std::stoull(v);
                } else {
                    return static_cast<uint64_t>(v);
                }
            }, value) + 1000;
        });
        std::cout << "ID at row 0 after adding 1000: " << table.get<uint64_t>(0, "ID") << "\n";

        table.add_column("NewID", uint64_t(1000));
        std::cout << "NewID at row 1: " << table.get<uint64_t>(1, "NewID") << "\n";

        table.sort_by_column<uint64_t>("ID", true);
        std::cout << "Table after sorting by ID:\n" << table << "\n";

        table.save_to_file("output.csv");
        std::cout << "Saved to output.csv\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}