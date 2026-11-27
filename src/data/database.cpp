#include "data/database.hpp"

#include "core/formatters.hpp"

#include "sqlite3.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

namespace data {
namespace {

class Statement {
public:
    explicit Statement(sqlite3* database, const char* sql) : statement_(nullptr) {
        if (database) {
            sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr);
        }
    }

    ~Statement() {
        if (statement_) {
            sqlite3_finalize(statement_);
        }
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    sqlite3_stmt* Get() const { return statement_; }
    explicit operator bool() const { return statement_ != nullptr; }

private:
    sqlite3_stmt* statement_;
};

std::string ColumnText(sqlite3_stmt* statement, int column) {
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : std::string();
}

bool ExecuteNoResult(sqlite3* database, const char* sql) {
    if (!database) {
        return false;
    }
    char* error = nullptr;
    const int result = sqlite3_exec(database, sql, nullptr, nullptr, &error);
    if (error) {
        sqlite3_free(error);
    }
    return result == SQLITE_OK;
}

std::string EscapeCSV(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) {
        return value;
    }
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (char character : value) {
        if (character == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(character);
    }
    escaped.push_back('"');
    return escaped;
}

std::vector<std::string> ParseCSVRow(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char character = line[i];
        if (quoted) {
            if (character == '"' && i + 1 < line.size() && line[i + 1] == '"') {
                field.push_back('"');
                ++i;
            } else if (character == '"') {
                quoted = false;
            } else {
                field.push_back(character);
            }
        } else if (character == '"') {
            quoted = true;
        } else if (character == ',') {
            fields.push_back(field);
            field.clear();
        } else {
            field.push_back(character);
        }
    }
    fields.push_back(field);
    return fields;
}

int64_t ParseInt64(const std::string& value) {
    if (value.empty()) {
        return 0;
    }
    try {
        return std::stoll(value);
    } catch (...) {
        return 0;
    }
}

} // namespace

DatabaseManager::DatabaseManager(const std::string& database_path)
    : database_path_(database_path) {}

DatabaseManager::~DatabaseManager() {
    Close();
}

void DatabaseManager::Close() {
    if (database_) {
        sqlite3_close(database_);
        database_ = nullptr;
    }
}

bool DatabaseManager::Initialize() {
    Close();
    if (sqlite3_open(database_path_.c_str(), &database_) != SQLITE_OK) {
        Close();
        return false;
    }

    if (!ExecuteNoResult(database_, "PRAGMA foreign_keys = ON;")) {
        return false;
    }

    constexpr const char* kCreateCategories =
        "CREATE TABLE IF NOT EXISTS categories ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL UNIQUE,"
        "color_hex TEXT NOT NULL"
        ");";
    constexpr const char* kCreateTransactions =
        "CREATE TABLE IF NOT EXISTS transactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "category_id INTEGER NOT NULL,"
        "amount_cents INTEGER NOT NULL,"
        "date TEXT NOT NULL,"
        "merchant TEXT NOT NULL,"
        "note TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(category_id) REFERENCES categories(id) ON DELETE CASCADE"
        ");";
    constexpr const char* kCreateBudgets =
        "CREATE TABLE IF NOT EXISTS budgets ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "category_id INTEGER NOT NULL,"
        "monthly_limit_cents INTEGER NOT NULL,"
        "month TEXT NOT NULL,"
        "UNIQUE(category_id, month),"
        "FOREIGN KEY(category_id) REFERENCES categories(id) ON DELETE CASCADE"
        ");";

    if (!ExecuteNoResult(database_, kCreateCategories) ||
        !ExecuteNoResult(database_, kCreateTransactions) ||
        !ExecuteNoResult(database_, kCreateBudgets)) {
        return false;
    }
    return true;
}

bool DatabaseManager::IsOpen() const {
    return database_ != nullptr;
}

std::vector<core::Category> DatabaseManager::GetCategories() {
    std::vector<core::Category> categories;
    if (!database_) {
        return categories;
    }

    Statement statement(database_,
        "SELECT id, name, color_hex FROM categories ORDER BY id;");
    if (!statement) {
        return categories;
    }
    while (sqlite3_step(statement.Get()) == SQLITE_ROW) {
        core::Category category;
        category.id = sqlite3_column_int64(statement.Get(), 0);
        category.name = ColumnText(statement.Get(), 1);
        category.color_hex = ColumnText(statement.Get(), 2);
        categories.push_back(std::move(category));
    }
    return categories;
}

std::vector<core::Transaction> DatabaseManager::QueryTransactions(
    const std::string& start_date,
    const std::string& end_date,
    int64_t category_id,
    const std::string& search_query) {
    std::vector<core::Transaction> transactions;
    if (!database_) {
        return transactions;
    }

    const std::string sql =
        "SELECT t.id, t.category_id, c.name, t.amount_cents, t.date, t.merchant, t.note "
        "FROM transactions t "
        "JOIN categories c ON c.id=t.category_id "
        "WHERE (?='' OR t.date>=?) "
        "AND (?='' OR t.date<=?) "
        "AND (?=0 OR t.category_id=?) "
        "AND (?='' OR lower(t.merchant) LIKE '%' || lower(?) || '%' "
        "OR lower(t.note) LIKE '%' || lower(?) || '%') "
        "ORDER BY t.date DESC, t.id DESC;";
    Statement statement(database_, sql.c_str());
    if (!statement) {
        return transactions;
    }

    sqlite3_bind_text(statement.Get(), 1, start_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 2, start_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 3, end_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 4, end_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement.Get(), 5, category_id);
    sqlite3_bind_int64(statement.Get(), 6, category_id);
    sqlite3_bind_text(statement.Get(), 7, search_query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 8, search_query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 9, search_query.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(statement.Get()) == SQLITE_ROW) {
        core::Transaction transaction;
        transaction.id = sqlite3_column_int64(statement.Get(), 0);
        transaction.category_id = sqlite3_column_int64(statement.Get(), 1);
        transaction.category_name = ColumnText(statement.Get(), 2);
        transaction.amount_cents = sqlite3_column_int64(statement.Get(), 3);
        transaction.date = ColumnText(statement.Get(), 4);
        transaction.merchant = ColumnText(statement.Get(), 5);
        transaction.note = ColumnText(statement.Get(), 6);
        transactions.push_back(std::move(transaction));
    }
    return transactions;
}

int64_t DatabaseManager::InsertTransaction(
    int64_t category_id,
    int64_t amount_cents,
    const std::string& date,
    const std::string& merchant,
    const std::string& note) {
    if (!database_) {
        return 0;
    }
    Statement statement(database_,
        "INSERT INTO transactions(category_id, amount_cents, date, merchant, note) "
        "VALUES(?, ?, ?, ?, ?);");
    if (!statement) {
        return 0;
    }
    sqlite3_bind_int64(statement.Get(), 1, category_id);
    sqlite3_bind_int64(statement.Get(), 2, amount_cents);
    sqlite3_bind_text(statement.Get(), 3, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 4, merchant.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 5, note.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement.Get()) != SQLITE_DONE) {
        return 0;
    }
    return sqlite3_last_insert_rowid(database_);
}

int64_t DatabaseManager::InsertTransaction(const core::Transaction& transaction) {
    return InsertTransaction(transaction.category_id, transaction.amount_cents,
        transaction.date, transaction.merchant, transaction.note);
}

bool DatabaseManager::UpdateTransaction(
    int64_t id,
    int64_t category_id,
    int64_t amount_cents,
    const std::string& date,
    const std::string& merchant,
    const std::string& note) {
    if (!database_) {
        return false;
    }
    Statement statement(database_,
        "UPDATE transactions SET category_id=?, amount_cents=?, date=?, "
        "merchant=?, note=? WHERE id=?;");
    if (!statement) {
        return false;
    }
    sqlite3_bind_int64(statement.Get(), 1, category_id);
    sqlite3_bind_int64(statement.Get(), 2, amount_cents);
    sqlite3_bind_text(statement.Get(), 3, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 4, merchant.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.Get(), 5, note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement.Get(), 6, id);
    return sqlite3_step(statement.Get()) == SQLITE_DONE &&
        sqlite3_changes(database_) > 0;
}

bool DatabaseManager::UpdateTransaction(const core::Transaction& transaction) {
    return UpdateTransaction(transaction.id, transaction.category_id,
        transaction.amount_cents, transaction.date,
        transaction.merchant, transaction.note);
}

bool DatabaseManager::DeleteTransaction(int64_t id) {
    if (!database_) {
        return false;
    }
    Statement statement(database_, "DELETE FROM transactions WHERE id=?;");
    if (!statement) {
        return false;
    }
    sqlite3_bind_int64(statement.Get(), 1, id);
    return sqlite3_step(statement.Get()) == SQLITE_DONE &&
        sqlite3_changes(database_) > 0;
}

std::vector<core::Budget> DatabaseManager::GetBudgetsForMonth(const std::string& month) {
    std::vector<core::Budget> budgets;
    if (!database_) {
        return budgets;
    }
    Statement statement(database_,
        "SELECT b.id, b.category_id, c.name, b.monthly_limit_cents, b.month "
        "FROM budgets b JOIN categories c ON c.id=b.category_id "
        "WHERE b.month=? ORDER BY c.name;");
    if (!statement) {
        return budgets;
    }
    sqlite3_bind_text(statement.Get(), 1, month.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(statement.Get()) == SQLITE_ROW) {
        core::Budget budget;
        budget.id = sqlite3_column_int64(statement.Get(), 0);
        budget.category_id = sqlite3_column_int64(statement.Get(), 1);
        budget.category_name = ColumnText(statement.Get(), 2);
        budget.monthly_limit_cents = sqlite3_column_int64(statement.Get(), 3);
        budget.month = ColumnText(statement.Get(), 4);
        budgets.push_back(std::move(budget));
    }
    return budgets;
}

bool DatabaseManager::UpsertBudget(
    int64_t category_id,
    int64_t monthly_limit_cents,
    const std::string& month) {
    if (!database_) {
        return false;
    }
    Statement statement(database_,
        "INSERT INTO budgets(category_id, monthly_limit_cents, month) "
        "VALUES(?, ?, ?) "
        "ON CONFLICT(category_id, month) "
        "DO UPDATE SET monthly_limit_cents=excluded.monthly_limit_cents;");
    if (!statement) {
        return false;
    }
    sqlite3_bind_int64(statement.Get(), 1, category_id);
    sqlite3_bind_int64(statement.Get(), 2, monthly_limit_cents);
    sqlite3_bind_text(statement.Get(), 3, month.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(statement.Get()) == SQLITE_DONE;
}

bool DatabaseManager::UpsertBudget(const core::Budget& budget) {
    return UpsertBudget(budget.category_id, budget.monthly_limit_cents, budget.month);
}

bool DatabaseManager::ExportToCSV(const std::string& filepath) {
    if (!database_) {
        return false;
    }
    std::ofstream output(filepath, std::ios::out | std::ios::trunc);
    if (!output) {
        return false;
    }

    output << "id,category_id,category_name,amount_cents,date,merchant,note\n";
    Statement statement(database_,
        "SELECT t.id, t.category_id, c.name, t.amount_cents, t.date, t.merchant, t.note "
        "FROM transactions t JOIN categories c ON c.id=t.category_id "
        "ORDER BY t.date, t.id;");
    if (!statement) {
        return false;
    }
    while (sqlite3_step(statement.Get()) == SQLITE_ROW) {
        output << sqlite3_column_int64(statement.Get(), 0) << ','
               << sqlite3_column_int64(statement.Get(), 1) << ','
               << EscapeCSV(ColumnText(statement.Get(), 2)) << ','
               << sqlite3_column_int64(statement.Get(), 3) << ','
               << EscapeCSV(ColumnText(statement.Get(), 4)) << ','
               << EscapeCSV(ColumnText(statement.Get(), 5)) << ','
               << EscapeCSV(ColumnText(statement.Get(), 6)) << '\n';
    }
    return static_cast<bool>(output);
}

bool DatabaseManager::ImportFromCSV(const std::string& filepath) {
    if (!database_) {
        return false;
    }
    std::ifstream input(filepath);
    if (!input) {
        return false;
    }

    std::string line;
    if (!std::getline(input, line)) {
        return false;
    }
    const std::vector<std::string> headers = ParseCSVRow(line);

    auto indexOf = [&headers](const std::string& name) -> std::size_t {
        for (std::size_t i = 0; i < headers.size(); ++i) {
            if (headers[i] == name) {
                return i;
            }
        }
        return headers.size();
    };

    const std::size_t category_id_index = indexOf("category_id");
    const std::size_t category_name_index = indexOf("category_name");
    const std::size_t amount_index = indexOf("amount_cents");
    const std::size_t date_index = indexOf("date");
    const std::size_t merchant_index = indexOf("merchant");
    const std::size_t note_index = indexOf("note");
    if (amount_index == headers.size() || date_index == headers.size() ||
        merchant_index == headers.size() ||
        (category_id_index == headers.size() && category_name_index == headers.size())) {
        return false;
    }

    if (!ExecuteNoResult(database_, "BEGIN;")) {
        return false;
    }
    bool success = true;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = ParseCSVRow(line);
        if (fields.size() < headers.size()) {
            success = false;
            break;
        }

        int64_t category_id = 0;
        if (category_id_index != headers.size()) {
            category_id = ParseInt64(fields[category_id_index]);
        } else {
            Statement lookup(database_, "SELECT id FROM categories WHERE name=?;");
            if (lookup) {
                sqlite3_bind_text(lookup.Get(), 1,
                    fields[category_name_index].c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(lookup.Get()) == SQLITE_ROW) {
                    category_id = sqlite3_column_int64(lookup.Get(), 0);
                }
            }
        }
        if (category_id == 0 ||
            InsertTransaction(category_id, ParseInt64(fields[amount_index]),
                fields[date_index], fields[merchant_index],
                note_index == headers.size() ? "" : fields[note_index]) == 0) {
            success = false;
            break;
        }
    }

    ExecuteNoResult(database_, success ? "COMMIT;" : "ROLLBACK;");
    return success;
}

bool DatabaseManager::SeedMockDataIfEmpty() {
    if (!database_ && !Initialize()) {
        return false;
    }
    if (!GetCategories().empty()) {
        return true;
    }

    const std::vector<std::pair<std::string, std::string>> defaults = {
        {"Housing", "#60A5FA"},
        {"Food & Dining", "#F97316"},
        {"Groceries", "#10B981"},
        {"Transport", "#A78BFA"},
        {"Utilities", "#FACC15"},
        {"Entertainment", "#EC4899"},
        {"Shopping", "#F43F5E"},
    };

    if (!ExecuteNoResult(database_, "BEGIN;")) {
        return false;
    }
    std::vector<int64_t> category_ids;
    bool success = true;

    for (const auto& [name, color] : defaults) {
        Statement statement(database_,
            "INSERT INTO categories(name, color_hex) VALUES(?, ?);");
        if (!statement) {
            success = false;
            break;
        }
        sqlite3_bind_text(statement.Get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.Get(), 2, color.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(statement.Get()) != SQLITE_DONE) {
            success = false;
            break;
        }
        category_ids.push_back(sqlite3_last_insert_rowid(database_));
    }

    if (success) {
        const std::string month = core::GetCurrentMonthISO();
        const int64_t limits[] = {180000, 45000, 60000, 35000, 30000, 25000, 50000};
        for (std::size_t i = 0; i < category_ids.size(); ++i) {
            if (!UpsertBudget(category_ids[i], limits[i], month)) {
                success = false;
                break;
            }
        }
    }

    if (success) {
        const std::vector<std::string> merchants = {
            "Lakeside Apartments", "Corner Bistro", "FreshMart",
            "Metro Transit", "City Energy", "Cinema House", "Urban Outfitters",
            "The Local Cafe", "Green Grocer", "Ride Share", "Water & Power",
            "Streaming Plus", "Electronics Depot"
        };
        const int64_t base_amounts[] = {
            145000, 2850, 9200, 2400, 11800, 3200, 6800
        };

        for (int i = 0; i < 150; ++i) {
            const int category_index = i % 7;
            int64_t amount = base_amounts[category_index];
            amount += static_cast<int64_t>((i * 7919) % 5200);

            if (i == 31) amount = 245000;
            if (i == 73) amount = 89999;
            if (i == 119) amount = 57500;
            if (i == 142) amount = 125000;

            std::tm date{};
            std::time_t now = std::time(nullptr);
            const std::tm* local = std::localtime(&now);
            if (local) {
                date = *local;
            }
            date.tm_mday -= (i * 37) % 181;
            date.tm_isdst = -1;
            std::mktime(&date);
            std::ostringstream date_stream;
            date_stream << std::setw(4) << std::setfill('0') << date.tm_year + 1900 << '-'
                        << std::setw(2) << std::setfill('0') << date.tm_mon + 1 << '-'
                        << std::setw(2) << std::setfill('0') << date.tm_mday;

            const std::string merchant =
                merchants[(i * 5 + category_index) % merchants.size()];
            std::string note;
            if (i == 31 || i == 73 || i == 119 || i == 142) {
                note = "Large purchase - anomaly candidate";
            }

            if (InsertTransaction(category_ids[category_index], amount,
                    date_stream.str(), merchant, note) == 0) {
                success = false;
                break;
            }
        }
    }

    ExecuteNoResult(database_, success ? "COMMIT;" : "ROLLBACK;");
    return success;
}

} // namespace data