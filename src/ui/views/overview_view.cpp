#include "ui/views/overview_view.hpp"

#include "core/formatters.hpp"
#include "imgui.h"
#include "implot.h"
#include "ui/components.hpp"
#include "ui/theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace ui {
namespace {

constexpr ImGuiTableFlags kTableFlags =
    ImGuiTableFlags_BordersInnerV |
    ImGuiTableFlags_BordersOuter |
    ImGuiTableFlags_RowBg |
    ImGuiTableFlags_SizingStretchProp |
    ImGuiTableFlags_ScrollY;

void RenderPaceChart(const analytics::DashboardStats& stats) {
    ImGui::BeginChild("##PaceChartCard", ImVec2(0.0f, 315.0f), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted("Cumulative Burn-Down");
    ImGui::Spacing();
    if (!stats.burn_days.empty() &&
        stats.burn_days.size() == stats.actual_cumulative_spend.size() &&
        stats.burn_days.size() == stats.ideal_budget_pace.size() &&
        ImPlot::BeginPlot("##BurnDown", ImVec2(-1.0f, 250.0f))) {
        ImPlot::SetupAxes("Day of month", "Cumulative spend");
        ImPlot::SetupAxisLimits(ImAxis_X1, 1.0, static_cast<double>(stats.burn_days.size()), ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0,
            std::max(1.0, *std::max_element(
                stats.actual_cumulative_spend.begin(),
                stats.actual_cumulative_spend.end()) * 1.15),
            ImGuiCond_Once);
        ImPlotSpec actual_spec{};
        actual_spec.LineColor = theme::kAccentPrimary;
        actual_spec.LineWeight = 2.5f;
        ImPlotSpec ideal_spec{};
        ideal_spec.LineColor = theme::kAccentEmerald;
        ideal_spec.LineWeight = 2.0f;
        ideal_spec.FillColor = ImVec4(
            theme::kAccentEmerald.x,
            theme::kAccentEmerald.y,
            theme::kAccentEmerald.z,
            0.12f);
        ImPlot::PlotShaded("Ideal Pace", stats.burn_days.data(),
            stats.ideal_budget_pace.data(),
            static_cast<int>(stats.burn_days.size()), 0.0, ideal_spec);
        ImPlot::PlotLine("Actual", stats.burn_days.data(),
            stats.actual_cumulative_spend.data(),
            static_cast<int>(stats.burn_days.size()), actual_spec);
        ImPlot::PlotLine("Ideal Pace", stats.burn_days.data(),
            stats.ideal_budget_pace.data(),
            static_cast<int>(stats.burn_days.size()), ideal_spec);
        ImPlot::EndPlot();
    } else {
        ImGui::TextColored(theme::kTextMuted, "No spending data for this month.");
    }
    ImGui::EndChild();
}

void RenderCategoryChart(const analytics::DashboardStats& stats) {
    ImGui::BeginChild("##CategoryChartCard", ImVec2(0.0f, 315.0f), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted("Category Breakdown");
    ImGui::Spacing();
    if (!stats.category_names.empty() &&
        stats.category_names.size() == stats.category_totals.size() &&
        ImPlot::BeginPlot("##CategoryBreakdown", ImVec2(-1.0f, 250.0f))) {
        std::vector<double> positions(stats.category_names.size());
        std::vector<const char*> labels;
        labels.reserve(stats.category_names.size());
        for (std::size_t i = 0; i < stats.category_names.size(); ++i) {
            positions[i] = static_cast<double>(i);
            labels.push_back(stats.category_names[i].c_str());
        }
        ImPlot::SetupAxes("Amount", "Category");
        ImPlot::SetupAxisTicks(ImAxis_Y1, positions.data(),
            static_cast<int>(positions.size()), labels.data());
        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5,
            static_cast<double>(positions.size()) - 0.5, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0,
            std::max(1.0, stats.category_totals.front() * 1.15), ImGuiCond_Once);
        ImPlotSpec horizontal_bars;
        horizontal_bars.Flags = ImPlotBarsFlags_Horizontal;
        ImPlot::PlotBars("Category spend", stats.category_totals.data(),
            static_cast<int>(stats.category_totals.size()), 0.65, 0.0,
            horizontal_bars);
        if (ImPlot::IsPlotHovered()) {
            const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
            const auto index = static_cast<std::ptrdiff_t>(std::llround(mouse.y));
            if (index >= 0 &&
                static_cast<std::size_t>(index) < stats.category_names.size() &&
                mouse.x >= 0.0 &&
                mouse.x <= stats.category_totals[static_cast<std::size_t>(index)]) {
                ImGui::BeginTooltip();
                ImGui::Text("%s: $%.2f (%.1f%%)",
                    stats.category_names[static_cast<std::size_t>(index)].c_str(),
                    stats.category_totals[static_cast<std::size_t>(index)],
                    stats.category_percentages[static_cast<std::size_t>(index)]);
                ImGui::EndTooltip();
            }
        }
        ImPlot::EndPlot();
    } else {
        ImGui::TextColored(theme::kTextMuted, "No category spending in this date range.");
    }
    ImGui::EndChild();
}

} // namespace

void RenderOverviewView(
    core::AppState& state,
    data::DatabaseManager& db,
    const analytics::DashboardStats& stats,
    const theme::FontSet& fonts) {
    ImGui::PushFont(fonts.bold ? fonts.bold : ImGui::GetFont());
    ImGui::TextUnformatted("Overview");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::TextColored(theme::kTextMuted, "Your spending at a glance");
    ImGui::Spacing();

    const ImVec4 delta_color = stats.period_delta_percent > 0.0
        ? theme::kAccentCoral
        : theme::kAccentEmerald;
    const std::string spent = core::FormatCurrency(stats.total_spent_cents, state.currency_symbol);
    const std::string delta = (stats.period_delta_percent > 0.0 ? "+" : "") +
        std::to_string(static_cast<int>(stats.period_delta_percent)) + "% vs previous period";
    const std::string daily = core::FormatCurrency(stats.daily_average_cents, state.currency_symbol);
    const std::string remaining = core::FormatCurrency(stats.remaining_budget_cents, state.currency_symbol);
    const std::string allowance = core::FormatCurrency(stats.safe_to_spend_per_day_cents, state.currency_symbol);

    if (ImGui::BeginTable("##OverviewKpis", 4,
            ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        RenderKpiCard("TOTAL SPENT", spent, delta, delta_color, fonts);
        ImGui::TableNextColumn();
        RenderKpiCard("DAILY AVERAGE", daily, "Average per day in selected range",
            theme::kAccentPrimary, fonts);
        ImGui::TableNextColumn();
        RenderKpiCard("REMAINING BUDGET", remaining, "For " + state.active_month,
            stats.remaining_budget_cents < 0 ? theme::kAccentCoral : theme::kAccentEmerald,
            fonts);
        ImGui::TableNextColumn();
        RenderKpiCard("SAFE DAILY ALLOWANCE", allowance, "Available per remaining day",
            theme::kAccentEmerald, fonts);
        ImGui::EndTable();
    }

    if (stats.total_budget_cents > 0) {
        const double spent_cents = static_cast<double>(
            stats.total_budget_cents - stats.remaining_budget_cents);
        const float fraction = static_cast<float>(std::clamp(
            spent_cents / static_cast<double>(stats.total_budget_cents), 0.0, 1.0));
        ImGui::TextColored(theme::kTextMuted, "Monthly budget progress");
        ImGui::ProgressBar(fraction, ImVec2(-1.0f, 12.0f), "");
    }

    ImGui::Spacing();
    if (ImGui::BeginTable("##OverviewCharts", 2,
            ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        RenderPaceChart(stats);
        ImGui::TableNextColumn();
        RenderCategoryChart(stats);
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Recent Transactions");
    ImGui::SameLine();
    ImGui::TextColored(theme::kTextMuted, "(latest 8)");
    if (ImGui::BeginTable("##RecentTransactions", 5, kTableFlags, ImVec2(0.0f, 230.0f))) {
        ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 105.0f);
        ImGui::TableSetupColumn("Merchant", ImGuiTableColumnFlags_WidthStretch, 1.6f);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 105.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        const auto& categories = stats.categories;
        const std::size_t count = std::min<std::size_t>(8, stats.transactions.size());
        for (std::size_t i = 0; i < count; ++i) {
            const core::Transaction& transaction = stats.transactions[i];
            ImGui::PushID(static_cast<int>(transaction.id));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(transaction.date.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(transaction.merchant.c_str());
            ImGui::TableSetColumnIndex(2);
            const auto category = std::find_if(categories.begin(), categories.end(),
                [&transaction](const core::Category& value) {
                    return value.id == transaction.category_id;
                });
            RenderCategoryPill(transaction.category_name,
                category == categories.end() ? "#38BDF8" : category->color_hex);
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(core::FormatCurrency(
                transaction.amount_cents, state.currency_symbol).c_str());
            ImGui::TableSetColumnIndex(4);
            if (ImGui::SmallButton("Edit")) {
                state.editing_transaction_id = transaction.id;
                state.open_add_expense_modal = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete")) {
                if (db.DeleteTransaction(transaction.id)) {
                    state.stats_dirty = true;
                    state.status_banner = "Transaction deleted.";
                } else {
                    state.status_banner = "Could not delete the transaction.";
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

} // namespace ui