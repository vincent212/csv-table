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
                if (pos == str.size())
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
                    return std::stoi(str);
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


        // Nested iterator class for rows
        class RowIterator {
        public:
            RowIterator(CSVTable* table, size_t index) : table_(table), index_(index) {}

            std::vector<CellValue>& operator*() const {
                return table_->rows[index_];
            }

            RowIterator& operator++() {
                ++index_;
                return *this;
            }

            bool operator!=(const RowIterator& other) const {
                return index_ != other.index_;
            }

        private:
            CSVTable* table_;
            size_t index_;
        };

        // Const iterator class for const CSVTable objects
        class ConstRowIterator {
        public:
            ConstRowIterator(const CSVTable* table, size_t index) : table_(table), index_(index) {}

            const std::vector<CellValue>& operator*() const {
                return table_->rows[index_];
            }

            ConstRowIterator& operator++() {
                ++index_;
                return *this;
            }

            bool operator!=(const ConstRowIterator& other) const {
                return index_ != other.index_;
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
                            cell = std::stoi(str);
                        }
                        else if constexpr (std::is_same_v<T, double>)
                        {
                            cell = std::stod(str);
                        }
                        else if constexpr (std::is_same_v<T, bool>)
                        {
                            if (str == "true")
                                cell = true;
                            else if (str == "false")
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
        void add_column(std::string_view col_name, const T &default_value)
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

    private:
        std::vector<std::string> col_names;
        std::map<std::string, int, std::less<>> col_map;
        std::vector<std::vector<CellValue>> rows;
    };

} // namespace m2