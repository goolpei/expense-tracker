#include "ui/app_shell.hpp"

#include "imgui.h"
#include "ui/theme.hpp"
#include "ui/views/analytics_view.hpp"
#include "ui/views/budgets_view.hpp"
#include "ui/views/expense_modal.hpp"
#include "ui/views/overview_view.hpp"
#include "ui/views/transactions_view.hpp"

#include <array>

namespace ui {
namespace {

constexpr std::array<const char*, 5> kDatePresetLabels = {
    "This month", "Last 30 days", "Last 6 months", "Year to date", "All time"
};
constexpr std::array<const char*, 4> kNavigationLabels = {
    "Overview", "Deep Analytics", "Transactions", "Budgets"
};

} // namespace

AppShell::AppShell(data::DatabaseManager& db, theme::FontSet fonts)
    : db_(db), fonts_(fonts) {}

void AppShell::Render(core::AppState& state) {
    if (state.stats_dirty) {
        stats_ = analytics::StatsEngine::Refresh(db_, state);
        state.stats_dirty = false;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags root_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("##ExpenseTrackerRoot", nullptr, root_flags);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::kBgSidebar);
    ImGui::BeginChild("##Sidebar", ImVec2(230.0f, 0.0f), ImGuiChildFlags_Borders);
    ImGui::PushFont(fonts_.bold ? fonts_.bold : ImGui::GetFont());
    ImGui::TextColored(theme::kAccentPrimary, "LEDGER // OS");
    ImGui::PopFont();
    ImGui::TextColored(theme::kTextMuted, "Personal Expense Analytics");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    for (std::size_t i = 0; i < kNavigationLabels.size(); ++i) {
        const bool selected = static_cast<std::size_t>(state.active_tab) == i;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImVec4(theme::kAccentPrimary.x, theme::kAccentPrimary.y,
                    theme::kAccentPrimary.z, 0.18f));
            ImGui::PushStyleColor(ImGuiCol_Text, theme::kAccentPrimary);
        }
        if (ImGui::Button(kNavigationLabels[i], ImVec2(-1.0f, 40.0f))) {
            state.active_tab = static_cast<core::NavTab>(i);
        }
        if (selected) {
            ImGui::PopStyleColor(2);
        }
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(theme::kTextMuted, "ACTIVE MONTH");
    ImGui::TextUnformatted(state.active_month.c_str());
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::kBgCanvas);
    ImGui::BeginChild("##MainContent", ImVec2(0.0f, 0.0f));
    ImGui::PushFont(fonts_.bold ? fonts_.bold : ImGui::GetFont());
    ImGui::TextUnformatted("EXPENSE TRACKER");
    ImGui::PopFont();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(170.0f);
    const int preset = static_cast<int>(state.date_preset);
    const char* current_preset = preset >= 0 &&
        preset < static_cast<int>(kDatePresetLabels.size())
        ? kDatePresetLabels[static_cast<std::size_t>(preset)]
        : kDatePresetLabels[0];
    if (ImGui::BeginCombo("##DateRangePreset", current_preset)) {
        for (std::size_t i = 0; i < kDatePresetLabels.size(); ++i) {
            if (ImGui::Selectable(kDatePresetLabels[i], preset == static_cast<int>(i))) {
                state.date_preset = static_cast<core::DateRangePreset>(i);
                state.stats_dirty = true;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("+ New expense")) {
        state.editing_transaction_id.reset();
        state.open_add_expense_modal = true;
    }

    if (!state.status_banner.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(theme::kAccentEmerald, "%s", state.status_banner.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) {
            state.status_banner.clear();
        }
    }
    ImGui::Separator();
    ImGui::BeginChild("##PageContent", ImVec2(0.0f, 0.0f));
    switch (state.active_tab) {
    case core::NavTab::Overview:
        RenderOverviewView(state, db_, stats_, fonts_);
        break;
    case core::NavTab::Analytics:
        RenderAnalyticsView(state, stats_);
        break;
    case core::NavTab::Transactions:
        RenderTransactionsView(state, db_, stats_, fonts_);
        break;
    case core::NavTab::Budgets:
        RenderBudgetsView(state, db_, stats_);
        break;
    }
    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::End();

    const ImGuiIO& io = ImGui::GetIO();
    if (!state.open_add_expense_modal &&
        io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
        state.editing_transaction_id.reset();
        state.open_add_expense_modal = true;
    }
    RenderExpenseModal(state, db_, stats_, fonts_);
}

} // namespace ui