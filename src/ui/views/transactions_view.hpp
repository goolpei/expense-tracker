#pragma once

#include "analytics/stats_engine.hpp"
#include "core/app_state.hpp"
#include "data/database.hpp"
#include "ui/theme.hpp"

namespace ui {

void RenderTransactionsView(
    core::AppState& state,
    data::DatabaseManager& db,
    const analytics::DashboardStats& stats,
    const theme::FontSet& fonts);

} // namespace ui
