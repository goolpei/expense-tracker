#pragma once

#include "analytics/stats_engine.hpp"
#include "core/app_state.hpp"
#include "data/database.hpp"
#include "ui/theme.hpp"

namespace ui {

class AppShell {
public:
    AppShell(data::DatabaseManager& db, theme::FontSet fonts);
    void Render(core::AppState& state);

private:
    data::DatabaseManager& db_;
    theme::FontSet fonts_;
    analytics::DashboardStats stats_;
};

} // namespace ui
