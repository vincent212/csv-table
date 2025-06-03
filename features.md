# CSVTable Functionality Summary

The `CSVTable` struct, implemented in C++23, is designed to read, manipulate, and write CSV files with column name and row index access. It uses `std::variant` for type-safe storage of cell values (`string`, `int`, `double`, `bool`) and provides a comprehensive set of operations for data manipulation. Below is a summary of its key functionalities:

## File I/O
- **Read CSV**: Loads a CSV file, parsing the first row as column names and subsequent rows as data, with automatic type inference (`string`, `int`, `double`, `bool`).
- **Write CSV**: Saves the table to a CSV file, preserving column names and formatting values appropriately.

## Data Access and Modification
- **Access Values**: Retrieve cell values by row index and column name using `get<T>` with type-safe casting.
- **Modify Values**: Assign values to specific cells using `table[row][col] = value` syntax via proxy objects (`CellProxy`, `CellAssigner`).
- **Set Column Type**: Convert string-based columns to specified types (e.g., `int`, `double`) with error handling or default values.

## Column Operations
- **Add Column**: Adds a new column with a default value.
- **Delete Column**: Removes a specified column, updating internal mappings.
- **Rename Columns**: Renames columns based on a provided mapping, ensuring no conflicts.

## Row Operations
- **Append Row**: Adds a new row, padding with empty strings if needed.
- **Filter Rows**: Returns indices or a new table with rows matching a predicate function.
- **Sub-table**: Creates a new table with selected rows.
- **Modify Rows**: Applies a user-defined function to modify rows in place.
- **Drop NA**: Removes rows with missing values (`"NA"`, `"NaN"`, `"#N/A"`, `""`) in specified or all columns.
- **Fill NA**: Replaces missing values with a specified value in selected columns.
- **Drop Duplicates**: Removes duplicate rows based on specified or all columns, keeping the first occurrence.

## Sorting
- **Sort by Column**: Sorts rows by a specified column in ascending or descending order, ensuring type consistency.

## Merging and Joining
- **Standard Merge**: Combines two tables based on common columns with join types (`"inner"`, `"left"`, `"right"`, `"outer"`).
- **First Merge**: Merges with another table, matching each row to the first row in the other table satisfying a predicate, with sorting options.
- **Predicate Merge**: Merges rows based on a custom predicate involving key columns and row indices.
- **Join**: Combines tables based on row indices with join types (`"inner"`, `"left"`, `"right"`, `"outer"`).

## Utility
- **Type-Safe Storage**: Uses `std::variant` for cell values, ensuring only supported types are stored.
- **Error Handling**: Throws exceptions for invalid inputs, type mismatches, or file errors.
- **Streaming**: Supports streaming the table to `std::ostream` for easy output.

## Implementation Notes
The implementation leverages C++23 features such as `std::ranges`, `std::string_view`, and concepts for improved performance, type safety, and modern syntax, replacing Boost dependencies with standard library equivalents.