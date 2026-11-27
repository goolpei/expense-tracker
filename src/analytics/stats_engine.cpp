#include "analytics/stats_engine.hpp"

#include "core/formatters.hpp"
#include "data/database.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace analytics {
namespace {

using Day = std::chrono::sys_days;
using namespace std::chrono;

std::optional<Day> ParseDate(const std::string& value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
        return std::nullopt;
    }

    int year_value = 0;
    unsigned month_value = 0;
    unsigned day_value = 0;
    const auto parse_part = [](const char* begin, const char* end, auto& output) {
        const auto result = std::from_chars(begin, end, output);
        return result.ec == std::errc{} && result.ptr == end;
    };
    if (!parse_part(value.data(), value.data() + 4, year_value) ||
        !parse_part(value.data() + 5, value.data() + 7, month_value) ||
        !parse_part(value.data() + 8, value.data() + 10, day_value)) {
        return std::nullopt;
    }

    const year_month_day date{year{year_value}, month{month_value}, day{day_value}};
    if (!date.ok()) {
        return std::nullopt;
    }
    return sys_days{date};
}

std::string FormatDate(Day date) {
    const year_month_day calendar_date{date};
    const auto two_digits = [](unsigned value) {
        return value < 10 ? "0" + std::to_string(value) : std::to_string(value);
    };
    return std::to_string(static_cast<int>(calendar_date.year())) + "-" +
        two_digits(static_cast<unsigned>(calendar_date.month())) + "-" +
        two_digits(static_cast<unsigned>(calendar_date.day()));
}

std::optional<year_month> ParseMonth(const std::string& value) {
    if (value.size() != 7 || value[4] != '-') {
        return std::nullopt;
    }

    int year_value = 0;
    unsigned month_value = 0;
    const auto year_result = std::from_chars(value.data(), value.data() + 4, year_value);
    const auto month_result = std::from_chars(value.data() + 5, value.data() + 7, month_value);
    if (year_result.ec != std::errc{} || year_result.ptr != value.data() + 4 ||
        month_result.ec != std::errc{} || month_result.ptr != value.data() + 7) {
        return std::nullopt;
    }

    const year_month parsed{year{year_value}, month{month_value}};
    return parsed.ok() ? std::optional<year_month>{parsed} : std::nullopt;
}

Day MonthStart(year_month value) {
    return sys_days{value / day{1}};
}

Day MonthEnd(year_month value) {
    return sys_days{value / last};
}

std::string FormatMonth(year_month value) {
    const int year_value = static_cast<int>(value.year());
    const unsigned month_value = static_cast<unsigned>(value.month());
    const std::string month_text = month_value < 10
        ? "0" + std::to_string(month_value)
        : std::to_string(month_value);
    return std::to_string(year_value) + "-" + month_text;
}

int64_t SaturatingAdd(int64_t left, int64_t right) {
    if (right > 0 && left > std::numeric_limits<int64_t>::max() - right) {
        return std::numeric_limits<int64_t>::max();
    }
    if (right < 0 && left < std::numeric_limits<int64_t>::min() - right) {
        return std::numeric_limits<int64_t>::min();
    }
    return left + right;
}

int64_t Difference(int64_t left, int64_t right) {
    if (right > 0 && left < std::numeric_limits<int64_t>::min() + right) {
        return std::numeric_limits<int64_t>::min();
    }
    if (right < 0 && left > std::numeric_limits<int64_t>::max() + right) {
        return std::numeric_limits<int64_t>::max();
    }
    return left - right;
}

int64_t SumAmounts(const std::vector<core::Transaction>& transactions) {
    int64_t total = 0;
    for (const core::Transaction& transaction : transactions) {
        total = SaturatingAdd(total, transaction.amount_cents);
    }
    return total;
}

double ToDollars(int64_t cents) {
    return static_cast<double>(cents) / 100.0;
}

std::size_t WeekdayIndex(Day date) {
    return static_cast<std::size_t>(weekday{date}.iso_encoding() - 1);
}

std::vector<core::Transaction> QueryTransactions(
    data::DatabaseManager& db,
    Day start,
    Day end,
    const core::AppState& state) {
    if (start > end || !db.IsOpen()) {
        return {};
    }
    return db.QueryTransactions(
        FormatDate(start), FormatDate(end), state.selected_category_id, state.search_query);
}

int64_t SumBudgets(data::DatabaseManager& db, const std::string& month, int64_t category_id) {
    int64_t total = 0;
    for (const core::Budget& budget : db.GetBudgetsForMonth(month)) {
        if (category_id == 0 || budget.category_id == category_id) {
            total = SaturatingAdd(total, budget.monthly_limit_cents);
        }
    }
    return total;
}

} // namespace

DashboardStats StatsEngine::Refresh(data::DatabaseManager& db, const core::AppState& state) {
    DashboardStats stats;
    if (!db.IsOpen()) {
        return stats;
    }

    const auto today_value = ParseDate(core::GetCurrentDateISO());
    if (!today_value) {
        return stats;
    }
    const Day today = *today_value;

    const core::DateRangeBounds preset_bounds = core::GetDateRangeBounds(state.date_preset);
    const auto range_start_value = ParseDate(preset_bounds.start_date);
    const auto range_end_value = ParseDate(preset_bounds.end_date);
    if (!range_start_value || !range_end_value) {
        return stats;
    }
    const Day range_start = *range_start_value;
    const Day range_end = std::min(*range_end_value, today);
    if (range_start > range_end) {
        return stats;
    }

    auto active_month_value = ParseMonth(state.active_month);
    if (!active_month_value) {
        active_month_value = ParseMonth(core::GetCurrentMonthISO());
    }
    if (!active_month_value) {
        return stats;
    }
    const year_month active_month = *active_month_value;
    stats.active_month = FormatMonth(active_month);
    const Day active_month_start = MonthStart(active_month);
    const Day active_month_end = MonthEnd(active_month);
    const Day active_month_to_date = std::min(active_month_end, today);

    stats.categories = db.GetCategories();
    std::unordered_map<int64_t, std::string> category_colors;
    for (const core::Category& category : stats.categories) {
        category_colors.emplace(category.id, category.color_hex);
    }

    std::vector<core::Transaction> filtered_transactions;
    if (state.date_preset == core::DateRangePreset::AllTime) {
        filtered_transactions = db.QueryTransactions(
            "", FormatDate(today), state.selected_category_id, state.search_query);
    } else {
        filtered_transactions = QueryTransactions(db, range_start, range_end, state);
    }

    Day effective_range_start = range_start;
    if (state.date_preset == core::DateRangePreset::AllTime && !filtered_transactions.empty()) {
        bool found_valid_date = false;
        for (const core::Transaction& transaction : filtered_transactions) {
            const auto transaction_date = ParseDate(transaction.date);
            if (transaction_date &&
                (!found_valid_date || *transaction_date < effective_range_start)) {
                effective_range_start = *transaction_date;
                found_valid_date = true;
            }
        }
        if (!found_valid_date) {
            effective_range_start = range_end;
        }
    }

    stats.total_spent_cents = SumAmounts(filtered_transactions);
    const auto selected_day_count = (range_end - effective_range_start).count() + 1;
    if (selected_day_count > 0) {
        stats.daily_average_cents =
            stats.total_spent_cents / static_cast<int64_t>(selected_day_count);
    }

    const auto period_days = (range_end - effective_range_start).count() + 1;
    const Day previous_period_end = effective_range_start - days{1};
    const Day previous_period_start = previous_period_end - days{period_days - 1};
    const auto previous_transactions =
        QueryTransactions(db, previous_period_start, previous_period_end, state);
    stats.previous_period_spent_cents = SumAmounts(previous_transactions);
    if (stats.previous_period_spent_cents != 0) {
        stats.period_delta_percent =
            (static_cast<double>(stats.total_spent_cents) -
                static_cast<double>(stats.previous_period_spent_cents)) /
            static_cast<double>(stats.previous_period_spent_cents) * 100.0;
    } else if (stats.total_spent_cents != 0) {
        stats.period_delta_percent = 100.0;
    }

    stats.transactions = std::move(filtered_transactions);
    std::unordered_map<int64_t, long double> category_sums;
    std::unordered_map<int64_t, std::size_t> category_counts;
    std::unordered_map<int64_t, int64_t> category_totals_cents;
    for (const core::Transaction& transaction : stats.transactions) {
        category_sums[transaction.category_id] +=
            static_cast<long double>(transaction.amount_cents);
        ++category_counts[transaction.category_id];
        category_totals_cents[transaction.category_id] =
            SaturatingAdd(category_totals_cents[transaction.category_id],
                transaction.amount_cents);
    }

    std::vector<std::pair<int64_t, int64_t>> ordered_categories(
        category_totals_cents.begin(), category_totals_cents.end());
    std::sort(ordered_categories.begin(), ordered_categories.end(),
        [](const auto& left, const auto& right) {
            if (left.second != right.second) {
                return left.second > right.second;
            }
            return left.first < right.first;
        });

    for (const auto& [category_id, total] : ordered_categories) {
        const auto transaction = std::find_if(
            stats.transactions.begin(), stats.transactions.end(),
            [category_id](const core::Transaction& value) {
                return value.category_id == category_id;
            });
        if (transaction == stats.transactions.end()) {
            continue;
        }
        stats.category_names.push_back(transaction->category_name);
        stats.category_totals.push_back(ToDollars(total));
        stats.category_percentages.push_back(
            stats.total_spent_cents == 0 ? 0.0 :
                static_cast<double>(total) /
                    static_cast<double>(stats.total_spent_cents) * 100.0);
        const auto color = category_colors.find(category_id);
        stats.category_colors_hex.push_back(
            color == category_colors.end() ? "#38BDF8" : color->second);
    }

    for (core::Transaction& transaction : stats.transactions) {
        const std::size_t count = category_counts[transaction.category_id];
        if (count == 0) {
            continue;
        }
        const long double mean =
            category_sums[transaction.category_id] / static_cast<long double>(count);
        transaction.is_anomaly =
            static_cast<long double>(transaction.amount_cents) > 2.0L * mean;
    }

    stats.all_transactions = db.QueryTransactions("", "", 0, "");
    std::unordered_map<int64_t, long double> all_category_sums;
    std::unordered_map<int64_t, std::size_t> all_category_counts;
    for (const core::Transaction& transaction : stats.all_transactions) {
        all_category_sums[transaction.category_id] +=
            static_cast<long double>(transaction.amount_cents);
        ++all_category_counts[transaction.category_id];
    }
    for (core::Transaction& transaction : stats.all_transactions) {
        const std::size_t count = all_category_counts[transaction.category_id];
        if (count != 0) {
            const long double mean =
                all_category_sums[transaction.category_id] /
                static_cast<long double>(count);
            transaction.is_anomaly =
                static_cast<long double>(transaction.amount_cents) > 2.0L * mean;
        }
    }

    stats.active_month_budgets = db.GetBudgetsForMonth(FormatMonth(active_month));
    for (const core::Budget& budget : stats.active_month_budgets) {
        if (state.selected_category_id == 0 ||
            budget.category_id == state.selected_category_id) {
            stats.total_budget_cents =
                SaturatingAdd(stats.total_budget_cents, budget.monthly_limit_cents);
        }
    }
    const auto month_transactions =
        QueryTransactions(db, active_month_start, active_month_to_date, state);
    const int64_t month_spent = SumAmounts(month_transactions);
    if (active_month_start <= active_month_to_date) {
        stats.active_month_transactions = db.QueryTransactions(
            FormatDate(active_month_start), FormatDate(active_month_to_date), 0, "");
    }
    stats.remaining_budget_cents =
        Difference(stats.total_budget_cents, month_spent);

    const unsigned days_in_active_month = static_cast<unsigned>(
        (active_month_end - active_month_start).count() + 1);
    int64_t elapsed_active_month_days = 0;
    int64_t remaining_active_month_days = 0;
    if (today >= active_month_start && today <= active_month_end) {
        elapsed_active_month_days = (today - active_month_start).count() + 1;
        remaining_active_month_days =
            static_cast<int64_t>(days_in_active_month) - elapsed_active_month_days + 1;
    } else if (today < active_month_start) {
        remaining_active_month_days = static_cast<int64_t>(days_in_active_month);
    } else {
        elapsed_active_month_days = static_cast<int64_t>(days_in_active_month);
    }
    if (remaining_active_month_days > 0) {
        stats.safe_to_spend_per_day_cents =
            stats.remaining_budget_cents / remaining_active_month_days;
    }
    if (elapsed_active_month_days > 0) {
        const long double projection =
            static_cast<long double>(month_spent) *
            static_cast<long double>(days_in_active_month) /
            static_cast<long double>(elapsed_active_month_days);
        stats.projected_month_end_cents =
            projection > static_cast<long double>(std::numeric_limits<int64_t>::max())
                ? std::numeric_limits<int64_t>::max()
                : projection < static_cast<long double>(std::numeric_limits<int64_t>::min())
                    ? std::numeric_limits<int64_t>::min()
                    : static_cast<int64_t>(projection);
    }

    stats.burn_days.reserve(days_in_active_month);
    stats.actual_cumulative_spend.reserve(days_in_active_month);
    stats.ideal_budget_pace.reserve(days_in_active_month);
    std::map<Day, int64_t> month_daily_totals;
    for (const core::Transaction& transaction : month_transactions) {
        const auto transaction_date = ParseDate(transaction.date);
        if (transaction_date) {
            month_daily_totals[*transaction_date] = SaturatingAdd(
                month_daily_totals[*transaction_date], transaction.amount_cents);
        }
    }
    int64_t cumulative_month_spend = 0;
    for (unsigned day_of_month = 1; day_of_month <= days_in_active_month; ++day_of_month) {
        const Day date = active_month_start + days{day_of_month - 1};
        const auto daily_total = month_daily_totals.find(date);
        if (daily_total != month_daily_totals.end()) {
            cumulative_month_spend =
                SaturatingAdd(cumulative_month_spend, daily_total->second);
        }
        stats.burn_days.push_back(static_cast<double>(day_of_month));
        stats.actual_cumulative_spend.push_back(ToDollars(cumulative_month_spend));
        stats.ideal_budget_pace.push_back(
            ToDollars(stats.total_budget_cents) *
            static_cast<double>(day_of_month) /
            static_cast<double>(days_in_active_month));
    }

    std::array<int64_t, 7> weekday_counts{};
    for (Day date = effective_range_start; date <= range_end; date += days{1}) {
        ++weekday_counts[WeekdayIndex(date)];
    }
    std::array<int64_t, 7> weekday_totals{};
    std::map<Day, int64_t> range_daily_totals;
    for (const core::Transaction& transaction : stats.transactions) {
        const auto transaction_date = ParseDate(transaction.date);
        if (!transaction_date) {
            continue;
        }
        range_daily_totals[*transaction_date] = SaturatingAdd(
            range_daily_totals[*transaction_date], transaction.amount_cents);
    }
    for (const auto& [date, total] : range_daily_totals) {
        weekday_totals[WeekdayIndex(date)] =
            SaturatingAdd(weekday_totals[WeekdayIndex(date)], total);
    }
    for (std::size_t weekday_index = 0; weekday_index < stats.weekday_avg_spend.size();
         ++weekday_index) {
        if (weekday_counts[weekday_index] > 0) {
            stats.weekday_avg_spend[weekday_index] =
                ToDollars(weekday_totals[weekday_index]) /
                static_cast<double>(weekday_counts[weekday_index]);
        }
    }

    const year_month first_chart_month = active_month - months{5};
    for (int month_offset = 0; month_offset < 6; ++month_offset) {
        const year_month month = first_chart_month + months{month_offset};
        const Day month_start = MonthStart(month);
        const Day month_end = std::min(MonthEnd(month), today);
        const auto month_values = QueryTransactions(db, month_start, month_end, state);
        stats.month_labels.push_back(FormatMonth(month));
        stats.monthly_actuals.push_back(ToDollars(SumAmounts(month_values)));
        stats.monthly_budgets.push_back(
            ToDollars(SumBudgets(db, FormatMonth(month), state.selected_category_id)));
    }

    const Day current_week_start = today - days{weekday{today}.iso_encoding() - 1};
    const Day heatmap_start = current_week_start - weeks{11};
    const Day heatmap_end = current_week_start + days{6};
    for (std::size_t weekday_index = 0; weekday_index < 7; ++weekday_index) {
        for (std::size_t week_index = 0; week_index < 12; ++week_index) {
            const std::size_t index = weekday_index * 12 + week_index;
            const Day date = heatmap_start + weeks{week_index} + days{weekday_index};
            stats.heatmap_dates[index] = FormatDate(date);
        }
    }
    const auto heatmap_transactions =
        QueryTransactions(db, heatmap_start, today, state);
    for (const core::Transaction& transaction : heatmap_transactions) {
        const auto transaction_date = ParseDate(transaction.date);
        if (!transaction_date || *transaction_date < heatmap_start ||
            *transaction_date > heatmap_end) {
            continue;
        }
        const std::size_t weekday_index = WeekdayIndex(*transaction_date);
        const auto week_index =
            static_cast<std::size_t>((*transaction_date - heatmap_start).count() / 7);
        if (week_index < 12) {
            const std::size_t index = weekday_index * 12 + week_index;
            stats.heatmap_values[index] += ToDollars(transaction.amount_cents);
        }
    }

    return stats;
}

} // namespace analytics
