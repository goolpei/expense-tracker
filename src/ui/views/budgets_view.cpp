#include "ui/views/budgets_view.hpp"

#include "core/formatters.hpp"
#include "imgui.h"
#include "ui/theme.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui {
namespace {

std::string FormatMonth(std::chrono::year_month month) {
    const int year = static_cast<int>(month.year());
    const unsigned month_number = static_cast<unsigned>(month.month());
    return std::to_string(year) + "-" +
        (month_number < 10 ? "0" : "") + std::to_string(month_number);
}

bool ParseAmount(const char* text, int64_t& cents) {
    if (!text || !*text) {
        return false;
    }
    const std::string value(text);
    const std::size_t decimal = value.find('.');
    const std::string dollars = value.substr(0, decimal);
    const std::string fraction = decimal == std::string::npos
        ? std::string()
        : value.substr(decimal + 1);
    if (dollars.empty() || fraction.size() > 2 ||
        (decimal != std::string::npos && value.find('.', decimal + 1) != std::string::npos)) {
        return false;
    }
    int64_t whole = 0;
    const auto parsed = std::from_chars(
        dollars.data(), dollars.data() + dollars.size(), whole);
    if (parsed.ec != std::errc{} || parsed.ptr != dollars.data() + dollars.size() ||
        whole < 0 || whole > (std::numeric_limits<int64_t>::max() / 100)) {
        return false;
    }
    int fraction_value = 0;
    if (!fraction.empty()) {
        const auto fractional = std::from_chars(
            fraction.data(), fraction.data() + fraction.size(), fraction_value);
        if (fractional.ec != std::errc{} ||
            fractional.ptr != fraction.data() + fraction.size()) {
            return false;
        }
        if (fraction.size() == 1) {
            fraction_value *= 10;
        }
    }
    if (whole > (std::numeric_limits<int64_t>::max() - fraction_value) / 100) {
        return false;
    }
    cents = whole * 100 + fraction_value;
    return true;
}

} // namespace

void RenderBudgetsView(
    core::AppState& state,
    data::DatabaseManager& db,
    const analytics::DashboardStats& stats) {
    ImGui::TextUnformatted("Monthly Budgets");
    ImGui::TextColored(theme::kTextMuted,
        "Set limits and track each category's current-month progress.");
    ImGui::Spacing();

    const std::string current_month = core::GetCurrentMonthISO();
    int current_year = 0;
    unsigned current_month_number = 1;
    std::from_chars(current_month.data(), current_month.data() + 4, current_year);
    std::from_chars(current_month.data() + 5, current_month.data() + 7, current_month_number);
    std::chrono::year_month month{
        std::chrono::year{current_year}, std::chrono::month{current_month_number}};
    bool parsed_active_month = false;
    if (state.active_month.size() == 7 && state.active_month[4] == '-') {
        int year = 0;
        unsigned month_number = 0;
        const auto year_result = std::from_chars(
            state.active_month.data(), state.active_month.data() + 4, year);
        const auto month_result = std::from_chars(
            state.active_month.data() + 5, state.active_month.data() + 7, month_number);
        const std::chrono::year_month candidate{
            std::chrono::year{year}, std::chrono::month{month_number}};
        if (year_result.ec == std::errc{} &&
            month_result.ec == std::errc{} && candidate.ok()) {
            month = candidate;
            parsed_active_month = true;
        }
    }
    if (!parsed_active_month) {
        state.active_month = FormatMonth(month);
    }
    if (stats.active_month != state.active_month) {
        ImGui::TextColored(theme::kTextMuted, "Updating budget data...");
        return;
    }

    if (ImGui::Button("< Previous")) {
        month -= std::chrono::months{1};
        state.active_month = FormatMonth(month);
        state.stats_dirty = true;
    }
    ImGui::SameLine();
    ImGui::Text("%s", state.active_month.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Next >")) {
        month += std::chrono::months{1};
        state.active_month = FormatMonth(month);
        state.stats_dirty = true;
    }

    std::unordered_map<int64_t, int64_t> spent_by_category;
    for (const core::Transaction& transaction : stats.active_month_transactions) {
        spent_by_category[transaction.category_id] += transaction.amount_cents;
    }

    const auto& categories = stats.categories;
    std::unordered_map<int64_t, int64_t> limits;
    for (const core::Budget& budget : stats.active_month_budgets) {
        limits[budget.category_id] = budget.monthly_limit_cents;
    }
    static std::unordered_map<int64_t, std::array<char, 32>> amount_buffers;
    static std::unordered_map<int64_t, std::string> loaded_months;

    ImGui::Spacing();
    for (const core::Category& category : categories) {
        int64_t& limit = limits[category.id];
        auto& buffer = amount_buffers[category.id];
        const std::string initial =
            std::to_string(limit / 100) + "." +
            (limit % 100 < 10 ? "0" : "") + std::to_string(limit % 100);
        if (loaded_months[category.id] != state.active_month) {
            buffer.fill('\0');
            std::copy_n(initial.c_str(),
                std::min(initial.size(), buffer.size() - 1), buffer.data());
            loaded_months[category.id] = state.active_month;
        }

        ImGui::PushID(static_cast<int>(category.id));
        ImGui::BeginChild("##BudgetCategory",
            ImVec2(0.0f, 105.0f), ImGuiChildFlags_Borders);
        ImGui::TextColored(theme::HexToImVec4(category.color_hex), "%s",
            category.name.c_str());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("Monthly limit ($)", buffer.data(), buffer.size());
        int64_t entered_limit = limit;
        const bool valid_amount = ParseAmount(buffer.data(), entered_limit);
        if (valid_amount) {
            limit = entered_limit;
        } else {
            ImGui::TextColored(theme::kAccentCoral, "Enter a valid budget amount.");
        }
        const int64_t spent = spent_by_category[category.id];
        const float fraction = limit <= 0 ? 0.0f :
            static_cast<float>(std::clamp(
                static_cast<double>(spent) / static_cast<double>(limit), 0.0, 1.0));
        ImGui::ProgressBar(fraction, ImVec2(-1.0f, 16.0f), "");
        ImGui::Text("%s spent of %s",
            core::FormatCurrency(spent, state.currency_symbol).c_str(),
            core::FormatCurrency(limit, state.currency_symbol).c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Save budget")) {
            if (!valid_amount) {
                state.status_banner = "Enter a valid budget amount before saving.";
            } else if (db.UpsertBudget(category.id, limit, state.active_month)) {
                state.status_banner = "Budget saved.";
                state.stats_dirty = true;
            } else {
                state.status_banner = "Could not save this budget.";
            }
        }
        ImGui::EndChild();
        ImGui::PopID();
        ImGui::Spacing();
    }
}

} // namespace ui