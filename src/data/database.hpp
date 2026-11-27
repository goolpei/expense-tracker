#pragma once

#include "core/models.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

namespace data {

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& database_path = "expenses.db");
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool Initialize();
    bool IsOpen() const;

    std::vector<core::Category> GetCategories();
    std::vector<core::Transaction> QueryTransactions(
        const std::string& start_date = "",
        const std::string& end_date = "",
        int64_t category_id = 0,
        const std::string& search_query = "");

    int64_t InsertTransaction(
        int64_t category_id,
        int64_t amount_cents,
        const std::string& date,
        const std::string& merchant,
        const std::string& note);
    int64_t InsertTransaction(const core::Transaction& transaction);

    bool UpdateTransaction(
        int64_t id,
        int64_t category_id,
        int64_t amount_cents,
        const std::string& date,
        const std::string& merchant,
        const std::string& note);
    bool UpdateTransaction(const core::Transaction& transaction);

    bool DeleteTransaction(int64_t id);

    std::vector<core::Budget> GetBudgetsForMonth(const std::string& month);
    bool UpsertBudget(int64_t category_id, int64_t monthly_limit_cents, const std::string& month);
    bool UpsertBudget(const core::Budget& budget);

    bool ExportToCSV(const std::string& filepath);
    bool ImportFromCSV(const std::string& filepath);
    bool SeedMockDataIfEmpty();

private:
    sqlite3* database_ = nullptr;
    std::string database_path_;

    void Close();
};

} // namespace data
