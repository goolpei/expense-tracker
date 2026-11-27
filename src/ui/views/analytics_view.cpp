#include "ui/views/analytics_view.hpp"

#include "imgui.h"
#include "implot.h"
#include "ui/theme.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace ui {
namespace {

void RenderMonthlyComparison(const analytics::DashboardStats& stats) {
    ImGui::BeginChild("##MonthlyComparison", ImVec2(0.0f, 340.0f), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted("6-Month Budget vs. Actual");
    ImGui::Spacing();
    if (stats.month_labels.size() == 6 &&
        stats.monthly_actuals.size() == 6 &&
        stats.monthly_budgets.size() == 6 &&
        ImPlot::BeginPlot("##MonthlyBudgetActual", ImVec2(-1.0f, 280.0f))) {
        std::array<double, 12> values{};
        for (std::size_t month = 0; month < 6; ++month) {
            values[month] = stats.monthly_budgets[month];
            values[month + 6] = stats.monthly_actuals[month];
        }
        const char* series[] = {"Budget", "Actual"};
        std::array<double, 6> month_positions{};
        std::array<const char*, 6> month_labels{};
        for (std::size_t i = 0; i < month_positions.size(); ++i) {
            month_positions[i] = static_cast<double>(i);
            month_labels[i] = stats.month_labels[i].c_str();
        }
        ImPlot::SetupAxes("Month", "Amount");
        ImPlot::SetupAxisTicks(ImAxis_X1, month_positions.data(),
            static_cast<int>(month_positions.size()), month_labels.data());
        ImPlot::PlotBarGroups(series, values.data(), 2, 6, 0.8);
        ImPlot::EndPlot();
    } else {
        ImGui::TextColored(theme::kTextMuted, "Monthly data is not available.");
    }
    ImGui::EndChild();
}

void RenderWeekdayAverages(const analytics::DashboardStats& stats) {
    static constexpr const char* kWeekdays[] = {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };
    ImGui::BeginChild("##WeekdayAverages", ImVec2(0.0f, 300.0f), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted("Average Spending by Day");
    ImGui::Spacing();
    if (ImPlot::BeginPlot("##WeekdaySpend", ImVec2(-1.0f, 240.0f))) {
        static constexpr double kPositions[] = {0, 1, 2, 3, 4, 5, 6};
        ImPlot::SetupAxes("Day of week", "Average amount");
        ImPlot::SetupAxisTicks(ImAxis_X1, kPositions, 7, kWeekdays);
        ImPlot::PlotBars("Average spend", stats.weekday_avg_spend.data(), 7, 0.65);
        ImPlot::EndPlot();
    }
    ImGui::EndChild();
}

void RenderSpendingHeatmap(const analytics::DashboardStats& stats) {
    static constexpr const char* kWeekdays[] = {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };
    std::array<double, 7> weekday_positions{};
    std::array<double, 12> week_positions{};
    std::array<std::string, 12> week_labels{};
    std::array<const char*, 12> week_label_pointers{};
    for (std::size_t i = 0; i < weekday_positions.size(); ++i) {
        weekday_positions[i] = static_cast<double>(i) + 0.5;
    }
    for (std::size_t i = 0; i < week_positions.size(); ++i) {
        week_positions[i] = static_cast<double>(i) + 0.5;
        week_labels[i] = std::to_string(i + 1);
        week_label_pointers[i] = week_labels[i].c_str();
    }

    ImGui::BeginChild("##SpendingIntensity", ImVec2(0.0f, 350.0f), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted("12-Week Spending Intensity");
    ImGui::Spacing();
    if (ImPlot::BeginPlot("##SpendingHeatmap", ImVec2(-1.0f, 285.0f))) {
        const double maximum = std::max(1.0,
            *std::max_element(stats.heatmap_values.begin(), stats.heatmap_values.end()));
        ImPlot::SetupAxes("Week (oldest to newest)", "Day");
        ImPlot::SetupAxisTicks(ImAxis_X1, week_positions.data(), 12,
            week_label_pointers.data());
        ImPlot::SetupAxisTicks(ImAxis_Y1, weekday_positions.data(), 7, kWeekdays);
        ImPlot::PlotHeatmap("Daily spend", stats.heatmap_values.data(),
            7, 12, 0.0, maximum, "%.0f",
            ImPlotPoint(0.0, 0.0), ImPlotPoint(12.0, 7.0));
        if (ImPlot::IsPlotHovered()) {
            const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
            const int week = static_cast<int>(std::floor(mouse.x));
            const int weekday = static_cast<int>(std::floor(mouse.y));
            if (week >= 0 && week < 12 && weekday >= 0 && weekday < 7) {
                const std::size_t index =
                    static_cast<std::size_t>(weekday * 12 + week);
                ImGui::BeginTooltip();
                ImGui::Text("%s: $%.2f", stats.heatmap_dates[index].c_str(),
                    stats.heatmap_values[index]);
                ImGui::EndTooltip();
            }
        }
        ImPlot::EndPlot();
    }
    ImGui::EndChild();
}

} // namespace

void RenderAnalyticsView(
    const core::AppState&,
    const analytics::DashboardStats& stats) {
    ImGui::TextUnformatted("Deep Analytics");
    ImGui::TextColored(theme::kTextMuted,
        "Budget adherence, weekly patterns, and day-by-day intensity.");
    ImGui::Spacing();
    RenderMonthlyComparison(stats);
    ImGui::Spacing();
    RenderWeekdayAverages(stats);
    ImGui::Spacing();
    RenderSpendingHeatmap(stats);
}

} // namespace ui