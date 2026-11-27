#pragma once

#include "analytics/stats_engine.hpp"
#include "core/app_state.hpp"
#include "ui/theme.hpp"

namespace ui {

void RenderAnalyticsView(
    const core::AppState& state,
    const analytics::DashboardStats& stats);

} // namespace ui
