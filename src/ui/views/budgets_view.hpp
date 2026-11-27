#pragma once

#include "analytics/stats_engine.hpp"
#include "core/app_state.hpp"
#include "data/database.hpp"

namespace ui {

void RenderBudgetsView(
    core::AppState& state,
    data::DatabaseManager& db,
    const analytics::DashboardStats& stats);

} // namespace ui
