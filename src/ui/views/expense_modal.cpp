#include "ui/views/expense_modal.hpp"

#include "core/formatters.hpp"
#include "imgui.h"
#include "ui/theme.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

namespace ui {
namespace {

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
        whole < 0 || whole > std::numeric_limits<int64_t>::max() / 100) {
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
    return cents > 0;
}

bool IsIsoDate(const char* date) {
    if (!date || std::char_traits<char>::length(date) != 10 ||
        date[4] != '-' || date[7] != '-') {
        return false;
    }
    for (int i = 0; i < 10; ++i) {
        if (i != 4 && i != 7 &&
            !std::isdigit(static_cast<unsigned char>(date[i]))) {
            return false;
        }
    }
    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    const auto year_result = std::from_chars(date, date + 4, year);
    const auto month_result = std::from_chars(date + 5, date + 7, month);
    const auto day_result = std::from_chars(date + 8, date + 10, day);
    const std::chrono::year_month_day parsed_date{
        std::chrono::year{year}, std::chrono::month{month},
        std::chrono::day{day}};
    return year_result.ec == std::errc{} && month_result.ec == std::errc{} &&
        day_result.ec == std::errc{} && parsed_date.ok();
}

template <std::size_t Size>
void SetText(std::array<char, Size>& buffer, const std::string& value) {
    const std::size_t length = std::min(value.size(), buffer.size() - 1);
    std::copy_n(value.data(), length, buffer.data());
    buffer[length] = '\0';
}

} // namespace

void RenderExpenseModal(
    core::AppState& state,
    data::DatabaseManager& db,
    const analytics::DashboardStats& stats,
    const theme::FontSet& fonts) {
    static bool was_open = false;
    static int64_t loaded_id = 0;
    static int64_t selected_category_id = 0;
    static std::array<char, 256> merchant{};
    static std::array<char, 256> note{};
    static std::array<char, 32> date{};
    static std::array<char, 48> amount{};
    static std::string error;

    const bool is_open = state.open_add_expense_modal;
    const int64_t requested_id = state.editing_transaction_id.value_or(0);
    if (is_open && (!was_open || loaded_id != requested_id)) {
        merchant.fill('\0');
        note.fill('\0');
        error.clear();
        selected_category_id = 0;
        SetText(date, core::GetCurrentDateISO());
        SetText(amount, "0.00");
        loaded_id = requested_id;
        if (requested_id != 0) {
            const auto transaction = std::find_if(
                stats.all_transactions.begin(), stats.all_transactions.end(),
                [requested_id](const core::Transaction& value) {
                    return value.id == requested_id;
                });
            if (transaction != stats.all_transactions.end()) {
                selected_category_id = transaction->category_id;
                SetText(merchant, transaction->merchant);
                SetText(note, transaction->note);
                SetText(date, transaction->date);
                const int64_t cents = transaction->amount_cents;
                SetText(amount, std::to_string(cents / 100) + "." +
                    (cents % 100 < 10 ? "0" : "") + std::to_string(cents % 100));
            }
        } else {
            const auto& categories = stats.categories;
            if (!categories.empty()) {
                selected_category_id = categories.front().id;
            }
        }
    }
    if (is_open && !was_open) {
        ImGui::OpenPopup("Expense Editor");
    }
    was_open = is_open;

    bool popup_open = is_open;
    if (ImGui::BeginPopupModal("Expense Editor", &popup_open,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        const auto& categories = stats.categories;
        ImGui::PushFont(fonts.bold ? fonts.bold : ImGui::GetFont());
        ImGui::TextUnformatted(requested_id == 0 ? "Add transaction" : "Edit transaction");
        ImGui::PopFont();
        ImGui::Spacing();
        ImGui::SetNextItemWidth(340.0f);
        ImGui::InputText("Merchant", merchant.data(), merchant.size());
        ImGui::SetNextItemWidth(340.0f);
        ImGui::InputText("Date (YYYY-MM-DD)", date.data(), date.size());
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputText("Amount ($)", amount.data(), amount.size());
        ImGui::SetNextItemWidth(340.0f);
        const core::Category* selected = nullptr;
        for (const core::Category& category : categories) {
            if (category.id == selected_category_id) {
                selected = &category;
                break;
            }
        }
        if (ImGui::BeginCombo("Category",
                selected ? selected->name.c_str() : "Select category")) {
            for (const core::Category& category : categories) {
                if (ImGui::Selectable(category.name.c_str(),
                        selected_category_id == category.id)) {
                    selected_category_id = category.id;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SetNextItemWidth(340.0f);
        ImGui::InputTextMultiline("Note", note.data(), note.size(),
            ImVec2(340.0f, 70.0f));
        if (!error.empty()) {
            ImGui::TextColored(theme::kAccentCoral, "%s", error.c_str());
        }

        if (ImGui::Button("Save", ImVec2(120.0f, 0.0f))) {
            int64_t amount_cents = 0;
            if (selected_category_id == 0) {
                error = "Choose a category.";
            } else if (merchant[0] == '\0') {
                error = "Merchant is required.";
            } else if (!IsIsoDate(date.data())) {
                error = "Enter a date in YYYY-MM-DD format.";
            } else if (!ParseAmount(amount.data(), amount_cents)) {
                error = "Enter an amount greater than zero with up to two decimals.";
            } else {
                const bool saved = requested_id == 0
                    ? db.InsertTransaction(selected_category_id, amount_cents,
                        date.data(), merchant.data(), note.data()) != 0
                    : db.UpdateTransaction(requested_id, selected_category_id,
                        amount_cents, date.data(), merchant.data(), note.data());
                if (saved) {
                    state.stats_dirty = true;
                    state.status_banner = requested_id == 0
                        ? "Transaction added."
                        : "Transaction updated.";
                    state.open_add_expense_modal = false;
                    state.editing_transaction_id.reset();
                    popup_open = false;
                    ImGui::CloseCurrentPopup();
                } else {
                    error = "Could not save the transaction.";
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            state.open_add_expense_modal = false;
            state.editing_transaction_id.reset();
            popup_open = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (!popup_open) {
        state.open_add_expense_modal = false;
        state.editing_transaction_id.reset();
    }
}

} // namespace ui