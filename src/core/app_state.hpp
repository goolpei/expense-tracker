#pragma once

#include "core/models.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace core {

struct AppState {
    // Navigation & active filters
    NavTab active_tab = NavTab::Overview;
    DateRangePreset date_preset = DateRangePreset::ThisMonth;
    int64_t selected_category_id = 0; // 0 = All categories
    std::string search_query;
    std::string active_month = "2026-11";

    // Modal & transient UI states
    bool open_add_expense_modal = false;
    std::optional<int64_t> editing_transaction_id;
    std::string status_banner;

    // Reactivity & lifecycle flags
    bool stats_dirty = true; // Views set this to true; AppShell refreshes StatsEngine once
    bool db_ready = false;

    // Display configuration
    std::string currency_symbol = "$";
};

} // namespace core