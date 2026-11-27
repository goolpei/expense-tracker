#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace core {

enum class NavTab {
    Overview = 0,
    Analytics,
    Transactions,
    Budgets
};

enum class DateRangePreset {
    ThisMonth = 0,
    Last30Days,
    Last6Months,
    YearToDate,
    AllTime
};

struct Category {
    int64_t id = 0;
    std::string name;
    std::string color_hex = "#38BDF8";
    std::array<float, 4> color_rgba = {0.22f, 0.74f, 0.97f, 1.0f};
};

struct Transaction {
    int64_t id = 0;
    int64_t category_id = 0;
    std::string category_name;
    int64_t amount_cents = 0; // Stored in integer cents to prevent floating-point drift
    std::string date;         // ISO-8601 format: "YYYY-MM-DD"
    std::string merchant;
    std::string note;
    bool is_anomaly = false;  // Flagged by StatsEngine when > 2x category mean
};

struct Budget {
    int64_t id = 0;
    int64_t category_id = 0;
    std::string category_name;
    int64_t monthly_limit_cents = 0;
    std::string month; // Format: "YYYY-MM"
};

} // namespace core