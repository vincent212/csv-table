#pragma once

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iomanip>
#include <cmath> // Added for std::floor

namespace m2
{
    /**
     * @brief Concept to ensure a type can be stored in CellValue.
     */
    template <typename T>
    concept ConvertibleToCellValue = std::same_as<T, std::string> ||
                                     std::same_as<T, int> ||
                                     std::same_as<T, double> ||
                                     std::same_as<T, bool> ||
                                     std::same_as<T, uint64_t>;

    /**
     * @brief A class to read, manipulate, and write CSV files with column name and row index access.
     *
     * This class reads a CSV file where the first row contains column names, stores values using std::variant for type safety,
     * and supports accessing/assigning values by row index and column name. It leverages C++23 features like std::optional,
     * std::string_view, and ranges for improved performance and clarity.
     *
     * @note Dependencies: Requires C++23 (compile with -std=c++23).
     * @note Error Handling: Functions throw exceptions for invalid inputs, type mismatches, or file errors.
     */
    class CSVTable
    {
    public:
        using CellValue = std::variant<std::string, int, double, bool, uint64_t>;

        /**
         * @brief Assigner class for setting cell values.
         */
        struct CellAssigner
        {
            CSVTable &table;
            int row_index;
            int col_index;

            CellAssigner(CSVTable &t, int ri, int ci) : table(t), row_index(ri), col_index(ci) {}

            /**
             * @brief Assigns a value to the cell.
             * @tparam T The type of the value to assign.
             * @param value The value to set.
             */
            template <ConvertibleToCellValue T>
            void operator=(const T &value)
            {
                table.rows[row_index][col_index] = value;
            }

            /**
             * @brief Streams the cell value to an output stream.
             * @param os The output stream.
             * @param assigner The assigner object.
             * @return std::ostream& The output stream.
             */
            friend std::ostream &operator<<(std::ostream &os, const CellAssigner &assigner)
            {
                if (assigner.row_index < 0 || static_cast<size_t>(assigner.row_index) >= assigner.table.rows.size() ||
                    assigner.col_index < 0 || static_cast<size_t>(assigner.col_index) >= assigner.table.col_names.size())
                {
                    os << "Invalid cell";
                }
                else
                {
                    os << CSVTable::cell_to_string(assigner.table.rows[assigner.row_index][assigner.col_index]);
                }
                return os;
            }
        };

        /**
         * @brief Proxy class for row-based indexing in the table.
         */
        struct CellProxy
        {
            CSVTable &table;
            int row_index;

            CellProxy(CSVTable &t, int ri) : table(t), row_index(ri) {}

            /**
             * @brief Accesses a column in the row.
             * @param col_name The name of the column.
             * @return CellAssigner An assigner for the cell at the specified row and column.
             * @throws std::out_of_range If the row index is invalid.
             * @throws std::invalid_argument If the column name is not found.
             */
            CellAssigner operator[](std::string_view col_name)
            {
                if (row_index < 0 || static_cast<size_t>(row_index) >= table.rows.size())
                {
                    throw std::out_of_range("Row index out of range");
                }
                auto it = table.col_map.find(col_name);
                if (it == table.col_map.end())
                {
                    throw std::invalid_argument("Column name not found: " + std::string(col_name));
                }
                return CellAssigner(table, row_index, it->second);
            }
        };

        /**
         * @brief Accesses a row in the table.
         * @param row_index The index of the row.
         * @return CellProxy A proxy for the specified row.
         */
        CellProxy operator[](int row_index)
        {
            return CellProxy(*this, row_index);
        }

        /**
         * @brief Converts a CellValue to its string representation.
         * @param value The CellValue to convert.
         * @return std::string The string representation of the value.
         */
        static std::string cell_to_string(const CellValue &value)
        {
            return std::visit([](const auto &v) -> std::string
                              {
                                  using T = std::decay_t<decltype(v)>;
                                  if constexpr (std::is_same_v<T, std::string>)
                                      return v;
                                  else if constexpr (std::is_same_v<T, int>)
                                      return std::to_string(v);
                                  else if constexpr (std::is_same_v<T, double>)
                                  {
                                      double d = v;
                                      if (d == std::floor(d))
                                          return std::to_string(static_cast<int>(d)); // No .0 for whole numbers
                                      std::ostringstream oss;
                                      oss << std::fixed << std::setprecision(10) << v;
                                      return oss.str();
                                  }
                                  else if constexpr (std::is_same_v<T, bool>)
                                      return v ? "true" : "false";
                                  else if constexpr (std::is_same_v<T, uint64_t>)
                                      return std::to_string(v);
                                  return "unknown"; },
                              value);
        }

        /**
         * @brief Parses a string to CellValue with type inference.
         * @param str The string to parse.
         * @return CellValue The parsed value.
         */
        static CellValue parse_cell(std::string_view str)
        {
            if (str.empty() || str == "NA" || str == "NaN" || str == "#N/A")
            {
                return std::string("");
            }
            if (str == "true")
                return true;
            if (str == "false")
                return false;
            try
            {
                size_t pos;
                int i = std::stoi(std::string(str), &pos);
                if (pos != str.size())
                    throw std::invalid_argument("stoi did not consume entire string");
                return i;
            }
            catch (...)
            {
            }
            try
            {
                size_t pos;
                uint64_t u = std::stoull(std::string(str), &pos);
                if (pos == str.size())
                    return u;
            }
            catch (...)
            {
            }
            try
            {
                size_t pos;
                double d = std::stod(std::string(str), &pos);
                if (pos == str.size())
                    return d;
            }
            catch (...)
            {
            }
            return std::string(str);
        }

        /**
         * @brief Converts a CellValue to type T, handling string conversions.
         * @param value The CellValue to convert.
         * @return T The converted value or default_value if conversion fails.
         */
        template <ConvertibleToCellValue T>
        static T convert_cell(const CellValue &value)
        {
            if (std::holds_alternative<T>(value))
            {
                return std::get<T>(value);
            }
            else if (std::holds_alternative<std::string>(value))
            {
                auto str = std::get<std::string>(value);
                if (str.empty() || str == "NA" || str == "NaN" || str == "#N/A")
                {
                    throw std::runtime_error("Cannot convert empty or NA string to type T");
                }
                if constexpr (std::is_same_v<T, int>)
                {
                    size_t pos;
                    int result = std::stoi(str, &pos);
                    if (pos != str.size())
                        throw std::invalid_argument("stoi did not consume entire string");
                    return result;
                }
                else if constexpr (std::is_same_v<T, double>)
                {
                    return std::stod(str);
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    if (str == "true")
                        return true;
                    if (str == "false")
                        return false;
                    if (str == "1")
                        return true;
                    if (str == "0")
                        return false;
                    throw std::runtime_error("Invalid boolean string: " + str);
                }
                else if constexpr (std::is_same_v<T, uint64_t>)
                {
                    return std::stoull(str);
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    return str;
                }
            }
            if constexpr (std::is_same_v<T, double>)
            {
                if (std::holds_alternative<int>(value))
                {
                    return static_cast<double>(std::get<int>(value));
                }
                else if (std::holds_alternative<uint64_t>(value))
                {
                    return static_cast<double>(std::get<uint64_t>(value));
                }
            }
            if constexpr (std::is_same_v<T, bool>)
            {
                if (std::holds_alternative<int>(value))
                {
                    return std::get<int>(value) != 0;
                }
                else if (std::holds_alternative<uint64_t>(value))
                {
                    return std::get<uint64_t>(value) != 0;
                }
            }
            if constexpr (std::is_same_v<T, uint64_t>)
            {
                if (std::holds_alternative<int>(value))
                {
                    return static_cast<uint64_t>(std::get<int>(value));
                }
                else if (std::holds_alternative<double>(value))
                {
                    return static_cast<uint64_t>(std::get<double>(value));
                }
            }
            throw std::runtime_error("Type mismatch: Cannot convert CellValue to the requested type");
        }

        /**
         * @brief Applies a function to each value in a specified column, transforming the values in place.
         *
         * This method applies the provided function `func` to each value in the column specified by `col_name`.
         * The column values are converted to type `T` if stored as strings, using a default value if conversion fails.
         *
         * @tparam T The type of the values in the column.
         * @tparam Func The type of the function to apply.
         * @param col_name The name of the column to transform.
         * @param func The function to apply to each value.
         * @throws std::invalid_argument If the column does not exist.
         */
        template <typename T, typename Func>
        void apply_to_column(std::string_view col_name, Func func)
        {
            auto it = col_map.find(std::string(col_name));
            if (it == col_map.end())
            {
                throw std::invalid_argument("Column not found: " + std::string(col_name));
            }
            int col_index = it->second;
            for (auto &row : rows)
            {
                try{
                    T value = convert_cell<T>(row[col_index]);
                    row[col_index] = func(value);
                }
                catch (const std::runtime_error &e)
                {
                    // conversion to type failed
                    row[col_index] = func("");
                }
            }
        }

        /**
         * @brief Constructor for an empty table.
         */
        CSVTable() = default;

        /**
         * @brief Constructor to load a CSV file.
         * @param filename The path to the CSV file.
         * @throws std::runtime_error If the file cannot be opened or is empty.
         */
        CSVTable(std::string_view filename)
        {
            read_file(filename);
        }

        /**
         * @brief Constructor for sub-tables or copies.
         * @param column_names The list of column names.
         * @param column_map The mapping from column names to indices.
         * @param selected_rows The selected rows of data.
         */
        CSVTable(const std::vector<std::string> &column_names,
                 const std::map<std::string, int, std::less<>> &column_map,
                 const std::vector<std::vector<CellValue>> &selected_rows)
            : col_names(column_names), col_map(column_map), rows(selected_rows) {}


        /**
         * @brief A proxy class for accessing and modifying a cell value in a Row.
         *
         * Allows reading and writing a cell value by reference, supporting type-safe
         * assignments to the cell in the parent CSVTable.
         */
        class CellAccess {
        public:
            /**
             * @brief Constructs a CellAccess for a specific cell.
             * @param table Pointer to the parent CSVTable.
             * @param row_index The index of the row.
             * @param col_index The index of the column.
             */
            CellAccess(CSVTable* table, size_t row_index, int col_index)
                : table_(table), row_index_(row_index), col_index_(col_index) {}

            /**
             * @brief Reads the cell value.
             * @return CellValue The current value of the cell.
             * @throws std::out_of_range If the row or column index is invalid.
             */
            operator CellValue() const {
                if (row_index_ >= table_->rows.size()) {
                    throw std::out_of_range("Row index out of range: " + std::to_string(row_index_));
                }
                if (col_index_ >= static_cast<int>(table_->rows[row_index_].size())) {
                    throw std::out_of_range("Column index out of range: " + std::to_string(col_index_));
                }
                return table_->rows[row_index_][col_index_];
            }

            /**
             * @brief Assigns a new value to the cell.
             * @tparam T The type of the value to assign (must be convertible to CellValue).
             * @param value The value to set.
             * @return CellAccess& Reference to this access object for chaining.
             * @throws std::out_of_range If the row or column index is invalid.
             */
            template <ConvertibleToCellValue T>
            CellAccess& operator=(const T& value) {
                if (row_index_ >= table_->rows.size()) {
                    throw std::out_of_range("Row index out of range: " + std::to_string(row_index_));
                }
                if (col_index_ >= static_cast<int>(table_->rows[row_index_].size())) {
                    throw std::out_of_range("Column index out of range: " + std::to_string(col_index_));
                }
                table_->rows[row_index_][col_index_] = value;
                return *this;
            }

        private:
            CSVTable* table_;      ///< Pointer to the parent table
            size_t row_index_;     ///< Index of the row
            int col_index_;        ///< Index of the column
        };

        /**
         * @brief A class representing a single row in a CSVTable.
         *
         * Allows access to cell values by column name using operator[], supporting both
         * reading and writing. Provides type-safe retrieval of values via get<T>.
         */
        class Row {
        public:
            /**
             * @brief Constructs a Row object.
             * @param table Pointer to the parent CSVTable.
             * @param row_index The index of the row in the table.
             */
            Row(CSVTable* table, size_t row_index)
                : table_(table), row_index_(row_index) {}

            /**
             * @brief Accesses a cell for reading or writing by column name.
             * @param col_name The name of the column.
             * @return CellAccess An access object for the cell, allowing read/write operations.
             * @throws std::invalid_argument If the column name does not exist.
             */
            CellAccess operator[](std::string_view col_name) {
                auto it = table_->col_map.find(col_name);
                if (it == table_->col_map.end()) {
                    throw std::invalid_argument("Column not found: " + std::string(col_name));
                }
                return CellAccess(table_, row_index_, it->second);
            }

            /**
             * @brief Retrieves a cell value with type safety.
             * @tparam T The type to retrieve (must be convertible to CellValue).
             * @param col_name The name of the column.
             * @return T The value converted to type T.
             * @throws std::invalid_argument If the column name does not exist.
             * @throws std::out_of_range If the row index is invalid.
             * @throws std::runtime_error If the value cannot be converted to T.
             */
            template <ConvertibleToCellValue T>
            T get(std::string_view col_name) const {
                return table_->get<T>(static_cast<int>(row_index_), col_name);
            }

            /**
             * @brief Streams the row to an output stream.
             *
             * Outputs the row as a comma-separated list of cell values, using the table's
             * cell_to_string function to convert values to strings.
             *
             * @param os The output stream to write to.
             * @param row The Row object to stream.
             * @return std::ostream& The output stream for chaining.
             * @throws std::out_of_range If the row index is invalid.
             */
            friend std::ostream& operator<<(std::ostream& os, const Row& row) {
                if (row.row_index_ >= row.table_->rows.size()) {
                    os << "<Invalid Row>";
                    return os;
                }
                const auto& row_data = row.table_->rows[row.row_index_];
                for (size_t i = 0; i < row_data.size(); ++i) {
                    os << CSVTable::cell_to_string(row_data[i]);
                    if (i < row_data.size() - 1) {
                        os << ",";
                    }
                }
                return os;
            }

        private:
            CSVTable* table_;      ///< Pointer to the parent table
            size_t row_index_;     ///< Index of the row
        };

        // Nested iterator class for rows
        class RowIterator {
        public:
            RowIterator(CSVTable* table, size_t index) : table_(table), index_(index) {}

            Row operator*() const {
                return Row(table_, index_);
            }

            Row operator->() const {
                return Row(table_, index_);
            }

            RowIterator& operator++() {
                ++index_;
                return *this;
            }

            bool operator!=(const RowIterator& other) const {
                return index_ != other.index_;
            }

            bool operator==(const RowIterator& other) const {
                return index_ == other.index_;
            }

            size_t index() const {
                return index_;
            }

            Row row() const {
                return Row(table_, index_);
            }

        private:
            CSVTable* table_;
            size_t index_;
        };

        // Const iterator class for const CSVTable objects
        class ConstRowIterator {
        public:
            ConstRowIterator(const CSVTable* table, size_t index) : table_(table), index_(index) {}

            Row operator*() const {
                return Row(const_cast<CSVTable*>(table_), index_);
            }

            Row operator->() const {
                return Row(const_cast<CSVTable*>(table_), index_);
            }

            ConstRowIterator& operator++() {
                ++index_;
                return *this;
            }

            bool operator!=(const ConstRowIterator& other) const {
                return index_ != other.index_;
            }

            bool operator==(const ConstRowIterator& other) const {
                return index_ == other.index_;
            }

            size_t index() const {
                return index_;
            }

            Row row() const {
                return Row(const_cast<CSVTable*>(table_), index_);
            }

        private:
            const CSVTable* table_;
            size_t index_;
        };

        // Begin and end methods for non-const objects
        RowIterator begin() {
            return RowIterator(this, 0);
        }

        RowIterator end() {
            return RowIterator(this, rows.size());
        }

        // Begin and end methods for const objects
        ConstRowIterator begin() const {
            return ConstRowIterator(this, 0);
        }

        ConstRowIterator end() const {
            return ConstRowIterator(this, rows.size());
        }

        ConstRowIterator cend() const {
            return end();
        }

        ConstRowIterator cbegin() const {
            return begin();
        }

         /**
         * @brief Reads a CSV file into the table, initializing or appending data.
         * @param filename The path to the CSV file.
         * @throws std::runtime_error If the file cannot be opened, is empty, or has mismatched columns when appending.
         */
        void read_file(std::string_view filename)
        {
            std::ifstream file(filename.data());
            if (!file.is_open())
            {
                throw std::runtime_error("Cannot open file: " + std::string(filename));
            }

            std::string line;
            if (!std::getline(file, line))
            {
                throw std::runtime_error("Empty file or missing header: " + std::string(filename));
            }

            // Parse header
            std::vector<std::string> new_col_names;
            auto fields = std::views::split(line, ',') |
                          std::views::transform([](auto&& rng)
                                                {
                                                    std::string s(rng.begin(), rng.end());
                                                    if (!s.empty() && s.front() == '"' && s.back() == '"')
                                                    {
                                                        s = s.substr(1, s.size() - 2);
                                                    }
                                                    return s;
                                                });
            for (const auto& field : fields)
            {
                new_col_names.push_back(field);
            }

            // If table is empty, initialize with new headers
            if (col_names.empty())
            {
                col_names = new_col_names;
                for (size_t i = 0; i < col_names.size(); ++i)
                {
                    col_map[col_names[i]] = i;
                }
            }
            // Otherwise, verify headers match
            else
            {
                if (new_col_names.size() != col_names.size() ||
                    !std::equal(new_col_names.begin(), new_col_names.end(), col_names.begin()))
                {
                    throw std::runtime_error("Column headers in " + std::string(filename) + " do not match existing table");
                }
            }

            // Read and append rows
            while (std::getline(file, line))
            {
                std::vector<CellValue> row;
                auto fields = std::views::split(line, ',') |
                              std::views::transform([](auto&& rng)
                                                    {
                                                        std::string s(rng.begin(), rng.end());
                                                        if (!s.empty() && s.front() == '"' && s.back() == '"')
                                                        {
                                                            s = s.substr(1, s.size() - 2);
                                                        }
                                                        return parse_cell(s);
                                                    });
                row.assign(fields.begin(), fields.end());
                while (row.size() < col_names.size())
                {
                    row.emplace_back(std::string(""));
                }
                rows.push_back(std::move(row));
            }
        }
    

        /**
         * @brief Sets the type of a column, converting values if possible.
         * @tparam T The target type for the column.
         * @param col_name The name of the column.
         * @param skip_errors If true, replace invalid values with default_value; otherwise, throw an exception.
         * @param default_value The default value to use for invalid conversions.
         * @throws std::invalid_argument If the column does not exist.
         * @throws std::runtime_error If conversion fails and skip_errors is false.
         */
        template <ConvertibleToCellValue T>
        void set_column_type(std::string_view col_name, bool skip_errors = false, const T &default_value = T())
        {
            auto it = col_map.find(col_name);
            if (it == col_map.end())
            {
                throw std::invalid_argument("Column not found: " + std::string(col_name));
            }
            int col_index = it->second;

            for (auto &row : rows)
            {
                auto &cell = row[col_index];
                if (std::holds_alternative<std::string>(cell))
                {
                    auto str = std::get<std::string>(cell);
                    if (str.empty() || str == "NA" || str == "NaN" || str == "#N/A")
                    {
                        if (skip_errors)
                        {
                            cell = default_value;
                        }
                        else
                        {
                            throw std::runtime_error("Invalid value in column " + std::string(col_name) + ": empty or NA");
                        }
                        continue;
                    }
                    try
                    {
                        if constexpr (std::is_same_v<T, int>)
                        {
                            size_t pos;
                            cell = std::stoi(str, &pos);
                            if (pos != str.size())
                                throw std::invalid_argument("stoi did not consume entire string");
                        }
                        else if constexpr (std::is_same_v<T, uint64_t>)
                        {
                            cell = std::stoull(str);
                        }
                        else if constexpr (std::is_same_v<T, double>)
                        {
                            cell = std::stod(str);
                        }
                        else if constexpr (std::is_same_v<T, bool>)
                        {
                            if (str == "true" ||"1" == str)
                                cell = true;
                            else if (str == "false" || "0" == str)
                                cell = false;
                            else
                                throw std::runtime_error("Invalid boolean value");
                        }
                        else if constexpr (std::is_same_v<T, std::string>)
                        {
                            cell = str;
                        }
                        else if constexpr (std::is_same_v<T, uint64_t>)
                        {
                            cell = std::stoull(str);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        if (skip_errors)
                        {
                            cell = default_value;
                        }
                        else
                        {
                            throw std::runtime_error("Conversion error in column " + std::string(col_name) + ": " + str);
                        }
                    }
                }
                else
                {
                    try
                    {
                        cell = convert_cell<T>(cell);
                    }
                    catch (const std::exception &e)
                    {
                        if (skip_errors)
                        {
                            cell = default_value;
                        }
                        else
                        {
                            throw std::runtime_error("Type mismatch in column " + std::string(col_name) + ": " + e.what());
                        }
                    }
                }
            }
        }

        /**
         * @brief Retrieves a value from the table with type safety.
         * @tparam T The type to retrieve.
         * @param row The row index.
         * @param col_name The column name.
         * @return T The value cast to type T.
         * @throws std::out_of_range If the row index is invalid.
         * @throws std::invalid_argument If the column does not exist.
         * @throws std::runtime_error If the type does not match or conversion fails.
         */
        template <ConvertibleToCellValue T>
        T get(int row, std::string_view col_name) const
        {
            if (row < 0 || static_cast<size_t>(row) >= rows.size())
            {
                throw std::out_of_range("Row index out of range");
            }
            auto it = col_map.find(col_name);
            if (it == col_map.end())
            {
                throw std::invalid_argument("Column name not found: " + std::string(col_name));
            }
            return convert_cell<T>(rows[row][it->second]);
        }

        /**
         * @brief Adds a new column with a default value.
         * @tparam T The type of the default value.
         * @param col_name The name of the new column.
         * @param default_value The default value for the new column.
         * @throws std::invalid_argument If the column already exists.
         */
        template <ConvertibleToCellValue T>
        void add_column(std::string_view col_name, const T &default_value = T())
        {
            if (col_map.contains(col_name))
            {
                throw std::invalid_argument("Column already exists: " + std::string(col_name));
            }
            col_map[std::string(col_name)] = col_names.size();
            col_names.emplace_back(col_name);
            for (auto &row : rows)
            {
                row.emplace_back(default_value);
            }
        }

        /**
         * @brief Deletes a column from the table.
         * @param col_name The name of the column to delete.
         * @throws std::invalid_argument If the column does not exist.
         */
        void delete_column(std::string_view col_name)
        {
            auto it = col_map.find(col_name);
            if (it == col_map.end())
            {
                throw std::invalid_argument("Column name not found: " + std::string(col_name));
            }
            int col_index = it->second;
            col_names.erase(col_names.begin() + col_index);
            col_map.erase(it);
            for (auto &[name, index] : col_map)
            {
                if (index > col_index)
                    --index;
            }
            for (auto &row : rows)
            {
                row.erase(row.begin() + col_index);
            }
        }

        /**
         * @brief Deletes multiple columns from the table.
         *
         * Removes the specified columns from the table by calling delete_column for each
         * column name. All specified columns must exist, or an exception is thrown.
         *
         * @param col_names A vector of column names to delete.
         * @throws std::invalid_argument If any column name does not exist.
         */
        void delete_columns(const std::vector<std::string_view>& col_names) {
            for (const auto& name : col_names) {
                delete_column(name);
            }
        }

        /**
         * @brief Appends a new row to the table.
         * @param values The values for the new row.
         */
        void append_row(std::vector<CellValue> values)
        {
            while (values.size() < col_names.size())
            {
                values.emplace_back(std::string(""));
            }
            values.resize(col_names.size());
            rows.push_back(std::move(values));
        }

        /**
         * @brief Filters rows based on a predicate.
         * @param predicate A function that takes a row index and the table, returning true if the row should be kept.
         * @return std::vector<int> The indices of the matching rows.
         */
        std::vector<int> filter_rows(const std::function<bool(int, const CSVTable &)> &predicate) const
        {
            std::vector<int> matching_rows;
            for (int i : std::views::iota(0, static_cast<int>(rows.size())))
            {
                if (predicate(i, *this))
                {
                    matching_rows.push_back(i);
                }
            }
            return matching_rows;
        }

        /**
         * @brief Creates a new table with rows that match a predicate.
         * @param predicate A function that takes a row index and the table, returning true if the row should be included.
         * @return CSVTable A new table with the filtered rows.
         */
        CSVTable filter_table(const std::function<bool(int, const CSVTable &)> &predicate) const
        {
            auto matching_rows = filter_rows(predicate);
            return sub_table(matching_rows);
        }

        /**
         * @brief Creates a sub-table with selected rows.
         * @param row_indices The indices of the rows to include.
         * @return CSVTable A new table with the selected rows.
         * @throws std::out_of_range If any row index is invalid.
         */
        CSVTable sub_table(const std::vector<int> &row_indices) const
        {
            std::vector<std::vector<CellValue>> selected_rows;
            for (int idx : row_indices)
            {
                if (static_cast<size_t>(idx) >= rows.size())
                {
                    throw std::out_of_range("Invalid row index: " + std::to_string(idx));
                }
                selected_rows.push_back(rows[idx]);
            }
            return CSVTable(col_names, col_map, selected_rows);
        }

        /**
         * @brief Modifies rows using a provided function.
         * @param modifier A function that takes a row index and the table, modifying the row in place.
         */
        void modify(const std::function<void(int, CSVTable &)> &modifier)
        {
            for (size_t i : std::views::iota(0u, rows.size()))
            {
                modifier(i, *this);
            }
        }

        /**
         * @brief Drops rows with missing values in specified columns.
         * @param columns The columns to check for missing values. If empty, checks all columns.
         * @throws std::invalid_argument If any specified column does not exist.
         */
        void dropna(const std::vector<std::string> &columns = {})
        {
            static constexpr std::array missing_values = {"NA", "NaN", "#N/A", ""};
            auto cols_to_check = columns.empty() ? col_names : columns;

            for (const auto &col : cols_to_check)
            {
                if (!col_map.contains(col))
                {
                    throw std::invalid_argument("Column name not found: " + col);
                }
            }

            std::vector<std::vector<CellValue>> new_rows;
            for (const auto &row : rows)
            {
                bool keep_row = true;
                for (const auto &col : cols_to_check)
                {
                    int col_index = col_map.at(col);
                    if (std::holds_alternative<std::string>(row[col_index]))
                    {
                        auto value = std::get<std::string>(row[col_index]);
                        if (std::ranges::contains(missing_values, value))
                        {
                            keep_row = false;
                            break;
                        }
                    }
                }
                if (keep_row)
                {
                    new_rows.push_back(row);
                }
            }
            rows = std::move(new_rows);
        }

        /**
         * @brief Renames columns based on a mapping.
         * @param rename_map A map from old column names to new column names.
         * @throws std::invalid_argument If an old column does not exist or a new column name already exists.
         */
        void rename_columns(const std::map<std::string, std::string, std::less<>> &rename_map)
        {
            for (const auto &[old_name, new_name] : rename_map)
            {
                auto it = col_map.find(old_name);
                if (it == col_map.end())
                {
                    throw std::invalid_argument("Column name not found: " + old_name);
                }
                if (col_map.contains(new_name))
                {
                    throw std::invalid_argument("New column name already exists: " + new_name);
                }
                int col_index = it->second;
                col_map.erase(it);
                col_map[new_name] = col_index;
                col_names[col_index] = new_name;
            }
        }

        /**
         * @brief Sorts the table by a column.
         * @tparam T The type of the column to sort by.
         * @param col_name The name of the column to sort by.
         * @param ascending If true, sort in ascending order; otherwise, descending.
         * @throws std::invalid_argument If the column does not exist.
         * @throws std::runtime_error If conversion fails.
         */
        template <ConvertibleToCellValue T>
        void sort_by_column(std::string_view col_name, bool ascending)
        {
            auto it = col_map.find(col_name);
            if (it == col_map.end())
            {
                throw std::invalid_argument("Column not found: " + std::string(col_name));
            }
            int col_index = it->second;

            std::ranges::sort(rows, [this, col_index, ascending](const auto &a, const auto &b)
                              {
                                  T val_a = convert_cell<T>(a[col_index]);
                                  T val_b = convert_cell<T>(b[col_index]);
                                  return ascending ? (val_a < val_b) : (val_a > val_b); });
        }

        /**
         * @brief Fills missing values in specified columns with a given value.
         * @tparam T The type of the fill value.
         * @param columns The columns to fill.
         * @param fill_value The value to use for filling.
         * @throws std::invalid_argument If any specified column does not exist.
         */
        template <ConvertibleToCellValue T>
        void fillna(const std::vector<std::string> &columns, const T &fill_value)
        {
            static constexpr std::array missing_values = {"NA", "NaN", "#N/A", ""};
            for (const auto &col : columns)
            {
                if (!col_map.contains(col))
                {
                    throw std::invalid_argument("Column name not found: " + col);
                }
                int col_index = col_map.at(col);
                for (auto &row : rows)
                {
                    if (std::holds_alternative<std::string>(row[col_index]))
                    {
                        auto value = std::get<std::string>(row[col_index]);
                        if (std::ranges::contains(missing_values, value))
                        {
                            row[col_index] = fill_value;
                        }
                    }
                }
            }
        }

        /**
         * @brief Drops duplicate rows based on specified columns.
         * @param columns The columns to consider for duplicates. If empty, considers all columns.
         * @throws std::invalid_argument If any specified column does not exist.
         */
        void drop_duplicates(const std::vector<std::string> &columns = {})
        {
            auto cols_to_check = columns.empty() ? col_names : columns;
            for (const auto &col : cols_to_check)
            {
                if (!col_map.contains(col))
                {
                    throw std::invalid_argument("Column name not found: " + col);
                }
            }

            std::unordered_set<std::string> seen;
            std::vector<std::vector<CellValue>> new_rows;
            for (const auto &row : rows)
            {
                std::stringstream key;
                for (const auto &col : cols_to_check)
                {
                    key << cell_to_string(row[col_map.at(col)]) << "|";
                }
                auto row_key = key.str();
                if (seen.insert(row_key).second)
                {
                    new_rows.push_back(row);
                }
            }
            rows = std::move(new_rows);
        }

        /**
         * @brief Merges this table with another table based on common columns.
         * @param other The other table to merge with.
         * @param on_columns The columns to merge on.
         * @param how The type of merge ("inner", "left", "right", "outer").
         * @return CSVTable The merged table.
         * @throws std::invalid_argument If on_columns are invalid or how is invalid.
         */
        CSVTable merge(const CSVTable &other, const std::vector<std::string> &on_columns, std::string_view how) const
        {
            if (!std::ranges::contains(std::array{"inner", "left", "right", "outer"}, how))
            {
                throw std::invalid_argument("Invalid join type: " + std::string(how));
            }

            for (const auto &col : on_columns)
            {
                if (!col_map.contains(col))
                {
                    throw std::invalid_argument("Column not found in left table: " + col);
                }
                if (!other.col_map.contains(col))
                {
                    throw std::invalid_argument("Column not found in right table: " + col);
                }
            }

            std::vector<std::string> new_col_names = col_names;
            std::map<std::string, int, std::less<>> new_col_map = col_map;
            std::vector<std::string> other_to_new_names;
            size_t non_key_cols = 0;
            for (const auto &col : other.col_names)
            {
                if (std::find(on_columns.begin(), on_columns.end(), col) == on_columns.end())
                {
                    std::string new_name = col;
                    int suffix = 0;
                    while (new_col_map.contains(new_name))
                    {
                        new_name = col + "_other" + (suffix ? std::to_string(suffix) : "");
                        suffix++;
                    }
                    other_to_new_names.push_back(new_name);
                    new_col_names.push_back(new_name);
                    new_col_map[new_name] = new_col_names.size() - 1;
                    non_key_cols++;
                }
                else
                {
                    other_to_new_names.push_back(col);
                }
            }

            std::vector<std::vector<CellValue>> new_rows;
            std::unordered_map<std::string, std::vector<size_t>> this_key_map;
            for (size_t i = 0; i < rows.size(); ++i)
            {
                std::stringstream key;
                for (const auto &col : on_columns)
                {
                    key << cell_to_string(rows[i][col_map.at(col)]) << "|";
                }
                this_key_map[key.str()].push_back(i);
            }

            std::unordered_map<std::string, std::vector<size_t>> other_key_map;
            for (size_t i = 0; i < other.rows.size(); ++i)
            {
                std::stringstream key;
                for (const auto &col : on_columns)
                {
                    key << cell_to_string(other.rows[i][other.col_map.at(col)]) << "|";
                }
                other_key_map[key.str()].push_back(i);
            }

            if (how == "inner" || how == "left" || how == "outer")
            {
                for (const auto &[key, this_indices] : this_key_map)
                {
                    bool has_match = other_key_map.contains(key);
                    auto other_indices = has_match ? other_key_map.at(key) : std::vector<size_t>{};

                    if (how == "inner" && !has_match)
                        continue;

                    for (size_t this_idx : this_indices)
                    {
                        if (has_match)
                        {
                            for (size_t other_idx : other_indices)
                            {
                                std::vector<CellValue> new_row(new_col_names.size());
                                for (const auto &col : col_names)
                                {
                                    new_row[new_col_map.at(col)] = rows[this_idx][col_map.at(col)];
                                }
                                for (size_t i = 0; i < other.col_names.size(); ++i)
                                {
                                    if (std::find(on_columns.begin(), on_columns.end(), other.col_names[i]) == on_columns.end())
                                    {
                                        new_row[new_col_map.at(other_to_new_names[i])] = other.rows[other_idx][i];
                                    }
                                }
                                new_rows.push_back(std::move(new_row));
                            }
                        }
                        else
                        {
                            std::vector<CellValue> new_row(new_col_names.size());
                            for (const auto &col : col_names)
                            {
                                new_row[new_col_map.at(col)] = rows[this_idx][col_map.at(col)];
                            }
                            for (size_t i = 0; i < non_key_cols; ++i)
                            {
                                new_row[new_col_map.at(other_to_new_names[on_columns.size() + i])] = std::string("");
                            }
                            new_rows.push_back(std::move(new_row));
                        }
                    }
                }
            }

            if (how == "right" || how == "outer")
            {
                for (const auto &[key, other_indices] : other_key_map)
                {
                    bool has_match = this_key_map.contains(key);
                    if (has_match && how != "outer")
                        continue;

                    for (size_t other_idx : other_indices)
                    {
                        std::vector<CellValue> new_row(new_col_names.size());
                        for (const auto &col : on_columns)
                        {
                            new_row[new_col_map.at(col)] = other.rows[other_idx][other.col_map.at(col)];
                        }
                        for (const auto &col : col_names)
                        {
                            if (std::find(on_columns.begin(), on_columns.end(), col) == on_columns.end())
                            {
                                new_row[new_col_map.at(col)] = std::string("");
                            }
                        }
                        for (size_t i = 0; i < other.col_names.size(); ++i)
                        {
                            if (std::find(on_columns.begin(), on_columns.end(), other.col_names[i]) == on_columns.end())
                            {
                                new_row[new_col_map.at(other_to_new_names[i])] = other.rows[other_idx][i];
                            }
                        }
                        new_rows.push_back(std::move(new_row));
                    }
                }
            }

            return CSVTable(new_col_names, new_col_map, new_rows);
        }

        /**
         * @brief Joins this table with another table based on row indices.
         * @param other The other table to join with.
         * @param how The type of join ("inner", "left", "right", "outer").
         * @return CSVTable The joined table.
         * @throws std::invalid_argument If how is invalid.
         */
        CSVTable join(const CSVTable &other, std::string_view how) const
        {
            if (!std::ranges::contains(std::array{"inner", "left", "right", "outer"}, how))
            {
                throw std::invalid_argument("Invalid join type: " + std::string(how));
            }

            std::vector<std::string> new_col_names;
            std::vector<std::pair<int, int>> column_sources;
            for (const auto &col : col_names)
            {
                new_col_names.push_back(col);
                column_sources.push_back({0, col_map.at(col)});
            }
            for (const auto &col : other.col_names)
            {
                std::string new_name = col;
                int suffix = 0;
                while (std::find(new_col_names.begin(), new_col_names.end(), new_name) != new_col_names.end())
                {
                    new_name = col + "_other" + (suffix ? std::to_string(suffix) : "");
                    suffix++;
                }
                new_col_names.push_back(new_name);
                column_sources.push_back({1, other.col_map.at(col)});
            }

            size_t N;
            if (how == "inner")
            {
                N = std::min(rows.size(), other.rows.size());
            }
            else if (how == "left")
            {
                N = rows.size();
            }
            else if (how == "right")
            {
                N = other.rows.size();
            }
            else
            {
                N = std::max(rows.size(), other.rows.size());
            }

            std::vector<CellValue> default_this(col_names.size(), std::string(""));
            std::vector<CellValue> default_other(other.col_names.size(), std::string(""));

            std::vector<std::vector<CellValue>> new_rows;
            for (size_t i = 0; i < N; ++i)
            {
                const std::vector<CellValue> &this_row = (i < rows.size()) ? rows[i] : default_this;
                const std::vector<CellValue> &other_row = (i < other.rows.size()) ? other.rows[i] : default_other;
                std::vector<CellValue> new_row;
                for (const auto &[table, orig_index] : column_sources)
                {
                    if (table == 0)
                    {
                        new_row.push_back(this_row[orig_index]);
                    }
                    else
                    {
                        new_row.push_back(other_row[orig_index]);
                    }
                }
                new_rows.push_back(std::move(new_row));
            }

            std::map<std::string, int, std::less<>> new_col_map;
            for (size_t i = 0; i < new_col_names.size(); ++i)
            {
                new_col_map[new_col_names[i]] = i;
            }

            return CSVTable(new_col_names, new_col_map, new_rows);
        }

        /**
         * @brief Saves the table to a CSV file.
         * @param filename The path to save the file.
         * @throws std::runtime_error If the file cannot be opened for writing.
         */
        void save_to_file(std::string_view filename) const
        {
            std::ofstream file(filename.data());
            if (!file.is_open())
            {
                throw std::runtime_error("Cannot open file for writing: " + std::string(filename));
            }

            // Write headers without quotes
            for (size_t i : std::views::iota(0u, col_names.size()))
            {
                file << col_names[i];
                if (i < col_names.size() - 1)
                    file << ",";
            }
            file << "\n";

            // Write rows without quotes
            for (const auto &row : rows)
            {
                for (size_t i : std::views::iota(0u, row.size()))
                {
                    file << cell_to_string(row[i]);
                    if (i < row.size() - 1)
                        file << ",";
                }
                file << "\n";
            }
        }

        /**
         * @brief Streams the table to an output stream.
         * @param os The output stream.
         * @param table The table to stream.
         * @return std::ostream& The output stream.
         */
        friend std::ostream &operator<<(std::ostream &os, const CSVTable &table)
        {
            for (size_t i : std::views::iota(0u, table.col_names.size()))
            {
                os << table.col_names[i];
                if (i < table.col_names.size() - 1)
                    os << ",";
            }
            os << "\n";
            for (const auto &row : table.rows)
            {
                for (size_t i : std::views::iota(0u, row.size()))
                {
                    os << cell_to_string(row[i]);
                    if (i < row.size() - 1)
                        os << ",";
                }
                os << "\n";
            }
            return os;
        }

        /**
         * @brief Gets the rows of the table.
         * @return std::vector<std::vector<CellValue>>& A reference to the rows.
         */
        std::vector<std::vector<CellValue>> &get_rows()
        {
            return rows;
        }

        /**
         * @brief Deletes a row from the table by its index.
         *
         * This function removes the row at the specified index from the table.
         * If the index is out of range, it throws a std::out_of_range exception.
         *
         * @param index The zero-based index of the row to delete.
         * @throws std::out_of_range If the index is greater than or equal to the number of rows.
         */
        void delete_row(size_t index) {
            if (index < rows.size()) {
                rows.erase(rows.begin() + index);
            } else {
                throw std::out_of_range("Index out of range");
            }
        }

        /**
         * @brief Removes rows from the table that satisfy a given condition.
         *
         * This function removes all rows from the table for which the provided condition function returns true.
         * The condition function should take a const reference to a row (std::vector<CellValue>) and return a boolean.
         *
         * @param condition A function that takes a row and returns true if the row should be removed.
         */
        void remove_rows(std::function<bool(const std::vector<CellValue>&)> condition) {
            rows.erase(std::remove_if(rows.begin(), rows.end(), condition), rows.end());
        }

        /**
         * @brief Gets the column names of the table.
         * @return const std::vector<std::string>& A reference to the column names.
         */
        auto get_col_names() const -> const std::vector<std::string> &
        {
            return col_names;
        }

        /**
         * @brief Gets the number of rows in the table.
         * @return size_t The number of rows.
         */
        size_t num_rows() const
        {
            return rows.size();
        }

        /**
         * @brief Appends the rows of another CSVTable to this table.
         * 
         * If this table is empty, it adopts the columns and rows of the other table.
         * If the other table is empty, no changes are made.
         * If both tables are non-empty, the columns must match exactly (same names and order).
         * 
         * @param other The CSVTable to append to this table.
         * @throws std::invalid_argument If the columns do not match when both tables are non-empty.
         */
        void append_table(const CSVTable& other) {
            if (other.col_names.empty()) {
                // Do nothing if the other table is empty
                return;
            }
            if (col_names.empty()) {
                // If this table is empty, set columns and rows from the other table
                col_names = other.col_names;
                col_map = other.col_map;
                rows = other.rows;
            } else {
                // Check if columns match exactly
                if (col_names != other.col_names) {
                    throw std::invalid_argument("Columns do not match for appending");
                }
                // Append rows from the other table
                rows.insert(rows.end(), other.rows.begin(), other.rows.end());
            }
        }


        /**
         * @brief Performs a binary search on the specified column to find the row with the value equal to the given value,
         * or the first row with a value greater than the given value if no exact match is found.
         *
         * The table must be sorted by the specified column using sort_by_column<T> with the same type T before calling this method.
         *
         * @tparam T The type to which the column values are converted for comparison.
         * @param col_name The name of the column to search on.
         * @param value The value to search for.
         * @return ConstRowIterator An iterator to the found row, or the end iterator if no such row exists.
         * @throws std::invalid_argument If the column name is not found.
         * @throws std::runtime_error If conversion of cell values fails.
         */
        template <typename T>
        ConstRowIterator lower_bound(std::string_view col_name, const T& value) const {
            auto it = col_map.find(col_name);
            if (it == col_map.end()) {
                throw std::invalid_argument("Column name not found: " + std::string(col_name));
            }
            int col_index = it->second;

            auto comp = [col_index](const std::vector<CellValue>& row, const T& val) {
                T row_val = CSVTable::convert_cell<T>(row[col_index]);
                return row_val < val;
            };

            auto vec_it = std::lower_bound(rows.cbegin(), rows.cend(), value, comp);
            size_t index = std::distance(rows.cbegin(), vec_it);
            return ConstRowIterator(this, index);
        }

        /**
         * @brief Finds a row in the table where the specified column matches the given value.
         *
         * This method performs a binary search on the specified column to locate a row
         * where the column's value matches the provided value. The column must be sorted
         * in ascending order for the search to work correctly.
         *
         * @tparam T The type of the value to search for. It must match the type of the column's values.
         * @param col_name The name of the column to search in.
         * @param value The value to search for in the specified column.
         * @return ConstRowIterator An iterator pointing to the row where the column value matches the given value.
         *         If no match is found, it returns the end iterator.
         * @throws std::invalid_argument If the specified column name does not exist in the table.
         */
        template <typename T>
        ConstRowIterator find(std::string_view col_name, const T& value) const {
            auto it = col_map.find(col_name);
            if (it == col_map.end()) {
                throw std::invalid_argument("Column not found: " + std::string(col_name));
            }
            int col_index = it->second;

            auto comp = [col_index](const auto& row, const T& val) {
                return CSVTable::convert_cell<T>(row[col_index]) < val;
            };

            auto vec_it = std::lower_bound(rows.begin(), rows.end(), value, comp);

            if (vec_it != rows.end() && CSVTable::convert_cell<T>((*vec_it)[col_index]) == value) {
                size_t index = std::distance(rows.begin(), vec_it);
                return ConstRowIterator(this, index);
            } else {
                return ConstRowIterator(this, rows.size());
            }
        }

        /**
         * @brief Sets all values in a specified column to a single value.
         * @tparam T The type of the value to set (must be convertible to CellValue).
         * @param col_name The name of the column to set.
         * @param value The value to set for the entire column.
         * @throws std::invalid_argument If the column does not exist.
         */
        template <ConvertibleToCellValue T>
        void set_column_to_value(std::string_view col_name, T value) {
            auto it = col_map.find(col_name);
            if (it == col_map.end()) {
                throw std::invalid_argument("Column not found: " + std::string(col_name));
            }
            int col_index = it->second;
            for (auto &row : rows) {
                row[col_index] = value;
            }
        }

        /**
        * @brief Returns a column as a vector of the specified type.
        * 
        * This function retrieves all values from the specified column and converts them
        * to the type T. The function assumes that all values in the column can be 
        * successfully converted to T. If the column does not exist, it throws a 
        * std::invalid_argument exception.
        * 
        * @tparam T The type to which the column values should be converted.
        * @param col_name The name of the column to retrieve.
        * @return std::vector<T> A vector containing the column values converted to type T.
        * @throws std::invalid_argument If the column does not exist.
        * @throws std::runtime_error If a value cannot be converted to type T.
        */
        template <ConvertibleToCellValue T>
        std::vector<T> get_column_as(std::string_view col_name) const {
            auto it = col_map.find(col_name);
            if (it == col_map.end()) {
                throw std::invalid_argument("Column not found: " + std::string(col_name));
            }
            int col_index = it->second;
            std::vector<T> column_values;
            column_values.reserve(rows.size());  // Reserve space for efficiency
            for (const auto& row : rows) {
                column_values.push_back(convert_cell<T>(row[col_index]));
            }
            return column_values;
        }

        /**
         * @brief Keeps every nth row in the table, removing all others.
         *
         * This function modifies the table in-place by retaining only the rows whose
         * zero-based indices are multiples of n (e.g., for n=2, keeps rows 0, 2, 4, etc.).
         * If n is 0, the table is cleared. If n is 1, all rows are kept (no change).
         *
         * @param n The interval at which to keep rows (must be non-negative).
         * @throws std::invalid_argument If n is negative.
         */
        void keep_every_nth_row(int n) {
            if (n < 0) {
                throw std::invalid_argument("n must be non-negative");
            }
            if (n == 0) {
                rows.clear();
                return;
            }
            if (n == 1) {
                return; // No change needed
            }

            std::vector<std::vector<CellValue>> new_rows;
            new_rows.reserve((rows.size() + n - 1) / n); // Reserve space for efficiency
            for (size_t i = 0; i < rows.size(); i += n) {
                new_rows.push_back(std::move(rows[i]));
            }
            rows = std::move(new_rows);
        }

    /**
    * @brief Calculates the mean of a column, assuming double values.
    *
    * Computes the arithmetic mean of all values in the specified column. The column
    * must contain values convertible to double.
    *
    * @param col_name The name of the column to compute the mean for.
    * @return double The mean value of the column.
    * @throws std::invalid_argument If the column does not exist or is empty.
    * @throws std::runtime_error If any value cannot be converted to double.
    */
    double mean(std::string_view col_name) const {
        auto column = get_column_as<double>(col_name);
        if (column.empty()) {
            throw std::invalid_argument("Cannot compute mean of empty column: " + std::string(col_name));
        }
        double sum = 0.0;
        for (double val : column) {
            sum += val;
        }
        return sum / column.size();
    }

    /**
     * @brief Calculates the median of a column, assuming double values.
     *
     * Computes the median of all values in the specified column by sorting the values
     * and finding the middle value (or average of two middle values for even-sized columns).
     * The column must contain values convertible to double.
     *
     * @param col_name The name of the column to compute the median for.
     * @return double The median value of the column.
     * @throws std::invalid_argument If the column does not exist or is empty.
     * @throws std::runtime_error If any value cannot be converted to double.
     */
    double median(std::string_view col_name) const {
        auto column = get_column_as<double>(col_name);
        if (column.empty()) {
            throw std::invalid_argument("Cannot compute median of empty column: " + std::string(col_name));
        }
        std::sort(column.begin(), column.end());
        size_t n = column.size();
        if (n % 2 == 0) {
            return (column[n / 2 - 1] + column[n / 2]) / 2.0;
        } else {
            return column[n / 2];
        }
    }

    /**
     * @brief Calculates the standard deviation of a column, assuming double values.
     *
     * Computes the sample standard deviation of all values in the specified column.
     * The column must contain values convertible to double.
     *
     * @param col_name The name of the column to compute the standard deviation for.
     * @return double The standard deviation of the column.
     * @throws std::invalid_argument If the column does not exist or has fewer than 2 values.
     * @throws std::runtime_error If any value cannot be converted to double.
     */
    double standard_deviation(std::string_view col_name) const {
        auto column = get_column_as<double>(col_name);
        if (column.size() < 2) {
            throw std::invalid_argument("Cannot compute standard deviation with fewer than 2 values in column: " + std::string(col_name));
        }
        double m = mean(col_name);
        double sum_sq_diff = 0.0;
        for (double val : column) {
            double diff = val - m;
            sum_sq_diff += diff * diff;
        }
        return std::sqrt(sum_sq_diff / (column.size() - 1));
    }

    /**
     * @brief Calculates the Pearson correlation coefficient between two columns.
     *
     * Computes the correlation between two specified columns, assuming both contain
     * values convertible to double. The coefficient ranges from -1 to 1, indicating
     * the strength and direction of the linear relationship.
     *
     * @param col_name1 The name of the first column.
     * @param col_name2 The name of the second column.
     * @return double The Pearson correlation coefficient.
     * @throws std::invalid_argument If either column does not exist, is empty, or columns have different sizes.
     * @throws std::runtime_error If any value cannot be converted to double or if the standard deviation of either column is zero.
     */
    double correlation(std::string_view col_name1, std::string_view col_name2) const {
        auto col1 = get_column_as<double>(col_name1);
        auto col2 = get_column_as<double>(col_name2);
        if (col1.empty()) {
            throw std::invalid_argument("Cannot compute correlation with empty column: " + std::string(col_name1));
        }
        if (col1.size() != col2.size()) {
            throw std::invalid_argument("Columns must have the same number of rows for correlation");
        }
        double mean1 = mean(col_name1);
        double mean2 = mean(col_name2);
        double std1 = standard_deviation(col_name1);
        double std2 = standard_deviation(col_name2);
        if (std1 == 0.0 || std2 == 0.0) {
            throw std::runtime_error("Cannot compute correlation with zero standard deviation");
        }
        double cov = 0.0;
        for (size_t i = 0; i < col1.size(); ++i) {
            cov += (col1[i] - mean1) * (col2[i] - mean2);
        }
        cov /= col1.size() - 1;
        return cov / (std1 * std2);
    }

    /**
     * @brief Calculates the R-squared (coefficient of determination) between two columns.
     *
     * Computes the R-squared value to measure the proportion of variance in the dependent
     * column (col_name2) explained by the independent column (col_name1). Both columns
     * must contain values convertible to double.
     *
     * @param col_name1 The name of the independent (predictor) column.
     * @param col_name2 The name of the dependent (actual) column.
     * @return double The R-squared value, typically between 0 and 1.
     * @throws std::invalid_argument If either column does not exist, is empty, or columns have different sizes.
     * @throws std::runtime_error If any value cannot be converted to double or if the variance of the dependent column is zero.
     */
    double r_squared(std::string_view col_name1, std::string_view col_name2) const {
        auto col1 = get_column_as<double>(col_name1); // Predicted values
        auto col2 = get_column_as<double>(col_name2); // Actual values
        if (col1.empty()) {
            throw std::invalid_argument("Cannot compute R-squared with empty column: " + std::string(col_name1));
        }
        if (col1.size() != col2.size()) {
            throw std::invalid_argument("Columns must have the same number of rows for R-squared");
        }
        double mean_y = mean(col_name2);
        double ss_tot = 0.0; // Total sum of squares
        double ss_res = 0.0; // Residual sum of squares
        for (size_t i = 0; i < col1.size(); ++i) {
            double y = col2[i]; // Actual
            double y_pred = col1[i]; // Predicted
            ss_tot += (y - mean_y) * (y - mean_y);
            ss_res += (y - y_pred) * (y - y_pred);
        }
        if (ss_tot == 0.0) {
            throw std::runtime_error("Cannot compute R-squared with zero total variance in column: " + std::string(col_name2));
        }
        return 1.0 - (ss_res / ss_tot);
    }

    /**
     * @brief Calculates the Root Mean Squared Error (RMSE) between two columns.
     *
     * Computes the RMSE to measure the average magnitude of errors between predicted
     * values in col_name1 and actual values in col_name2. Both columns must contain
     * values convertible to double.
     *
     * @param col_name1 The name of the predicted column.
     * @param col_name2 The name of the actual column.
     * @return double The RMSE value.
     * @throws std::invalid_argument If either column does not exist, is empty, or columns have different sizes.
     * @throws std::runtime_error If any value cannot be converted to double.
     */
    double rmse(std::string_view col_name1, std::string_view col_name2) const {
        auto col1 = get_column_as<double>(col_name1); // Predicted values
        auto col2 = get_column_as<double>(col_name2); // Actual values
        if (col1.empty()) {
            throw std::invalid_argument("Cannot compute RMSE with empty column: " + std::string(col_name1));
        }
        if (col1.size() != col2.size()) {
            throw std::invalid_argument("Columns must have the same number of rows for RMSE");
        }
        double sum_sq_error = 0.0;
        for (size_t i = 0; i < col1.size(); ++i) {
            double error = col1[i] - col2[i];
            sum_sq_error += error * error;
        }
        return std::sqrt(sum_sq_error / col1.size());
    }

    /**
     * @brief Calculates the squared error of a column, assuming a mean of zero.
     *
     * Computes the sum of squared values in the specified column, assuming the mean is zero
     * (i.e., no mean subtraction). The column must contain values convertible to double.
     *
     * @param col_name The name of the column to compute the squared error for.
     * @return double The sum of squared values in the column.
     * @throws std::invalid_argument If the column does not exist or is empty.
     * @throws std::runtime_error If any value cannot be converted to double.
     */
    double squared_error(std::string_view col_name) const {
        auto column = get_column_as<double>(col_name);
        if (column.empty()) {
            throw std::invalid_argument("Cannot compute squared error of empty column: " + std::string(col_name));
        }
        double sum_sq = 0.0;
        for (double val : column) {
            sum_sq += val * val;
        }
        return sum_sq;
    }

    /**
     * @brief Calculates the specified percentile of a column, assuming double values.
     *
     * Computes the percentile (e.g., 0.25 for 25th percentile) of the values in the specified
     * column. The column must contain values convertible to double. Linear interpolation is used
     * if the percentile falls between two values.
     *
     * @param col_name The name of the column to compute the percentile for.
     * @param p The percentile to compute, in the range [0, 1] (e.g., 0.25 for 25th percentile).
     * @return double The value at the specified percentile.
     * @throws std::invalid_argument If the column does not exist, is empty, or p is not in [0, 1].
     * @throws std::runtime_error If any value cannot be converted to double.
     */
    double percentile(std::string_view col_name, double p) const {
        if (p < 0.0 || p > 1.0) {
            throw std::invalid_argument("Percentile p must be in [0, 1], got: " + std::to_string(p));
        }
        auto column = get_column_as<double>(col_name);
        if (column.empty()) {
            throw std::invalid_argument("Cannot compute percentile of empty column: " + std::string(col_name));
        }
        std::sort(column.begin(), column.end());
        size_t n = column.size();
        double index = p * (n - 1);
        size_t lower_idx = static_cast<size_t>(std::floor(index));
        if (lower_idx == n - 1) {
            return column[n - 1];
        }
        double fraction = index - lower_idx;
        return column[lower_idx] + fraction * (column[lower_idx + 1] - column[lower_idx]);
    }

    /**
     * @brief Retrieves a Row object for the specified row index.
     *
     * Returns a Row object that allows access to the row's values by column name.
     *
     * @param index The zero-based index of the row.
     * @return Row A Row object representing the specified row.
     * @throws std::out_of_range If the index is out of range.
     */
    Row get_row(size_t index)  {
        if (index >= rows.size()) {
            throw std::out_of_range("Row index out of range: " + std::to_string(index));
        }
        return Row(this, index);
    }

    /**
     * @brief Removes rows from the table that satisfy a predicate.
     *
     * Iterates over the table's rows and removes those for which the provided predicate
     * returns true. The predicate takes a Row object representing the current row.
     *
     * @param predicate A function that takes a Row object and returns true if the row
     *                  should be removed.
     */
    void remove_rows_if(std::function<bool(const Row&)> predicate) {
        std::vector<std::vector<CellValue>> new_rows;
        new_rows.reserve(rows.size()); // Reserve space for efficiency
        for (size_t i = 0; i < rows.size(); ++i) {
            Row row(this, i);
            if (!predicate(row)) {
                new_rows.push_back(std::move(rows[i]));
            }
        }
        rows = std::move(new_rows);
    }
    
    private:
        std::vector<std::string> col_names;
        std::map<std::string, int, std::less<>> col_map;
        std::vector<std::vector<CellValue>> rows;
    };

} // namespace m2