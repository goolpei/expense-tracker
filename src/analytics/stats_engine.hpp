#pragma once

#include "core/app_state.hpp"
#include "core/models.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace data {
class DatabaseManager;
}

namespace analytics {

struct DashboardStats {
    int64_t total_spent_cents = 0;
    int64_t previous_period_spent_cents = 0;
    double period_delta_percent = 0.0;
    int64_t daily_average_cents = 0;
    int64_t total_budget_cents = 0;
    int64_t remaining_budget_cents = 0;
    int64_t safe_to_spend_per_day_cents = 0;
    int64_t projected_month_end_cents = 0;

    std::vector<double> burn_days;
    std::vector<double> actual_cumulative_spend;
    std::vector<double> ideal_budget_pace;

    std::vector<std::string> category_names;
    std::vector<double> category_totals;
    std::vector<double> category_percentages;
    std::vector<std::string> category_colors_hex;

    std::vector<std::string> month_labels;
    std::vector<double> monthly_actuals;
    std::vector<double> monthly_budgets;

    std::array<double, 7> weekday_avg_spend{};
    std::array<double, 7 * 12> heatmap_values{};
    std::array<std::string, 7 * 12> heatmap_dates{};

    std::vector<core::Transaction> transactions;
};

class StatsEngine {
public:
    static DashboardStats Refresh(data::DatabaseManager& db, const core::AppState& state);
};

} // namespace analytics
