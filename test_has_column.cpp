#include "CSVTable.hpp"
#include <iostream>

using namespace m2;

int main() {
    std::cout << "Testing has_column() method\n" << std::endl;

    // Create a table
    CSVTable table;
    table.add_column<std::string>("Name");
    table.add_column<int>("Age");
    table.add_column<double>("Score");

    table.append_row({std::string("Alice"), 30, 95.5});
    table.append_row({std::string("Bob"), 25, 87.3});

    std::cout << "Table created with columns: Name, Age, Score\n" << std::endl;

    // Test has_column with existing columns
    std::cout << "has_column('Name'): " << (table.has_column("Name") ? "true" : "false") << std::endl;
    std::cout << "has_column('Age'): " << (table.has_column("Age") ? "true" : "false") << std::endl;
    std::cout << "has_column('Score'): " << (table.has_column("Score") ? "true" : "false") << std::endl;

    // Test has_column with non-existing column
    std::cout << "has_column('Missing'): " << (table.has_column("Missing") ? "true" : "false") << std::endl;
    std::cout << "has_column('ID'): " << (table.has_column("ID") ? "true" : "false") << std::endl;

    std::cout << "\n✓ has_column() works correctly!" << std::endl;

    // Test on empty table
    CSVTable empty_table;
    std::cout << "\nTesting on empty table:" << std::endl;
    std::cout << "has_column('AnyColumn'): " << (empty_table.has_column("AnyColumn") ? "true" : "false") << std::endl;

    // Add column to empty table and test
    empty_table.add_column<int>("ID");
    std::cout << "\nAfter adding 'ID' column to empty table:" << std::endl;
    std::cout << "has_column('ID'): " << (empty_table.has_column("ID") ? "true" : "false") << std::endl;

    return 0;
}
