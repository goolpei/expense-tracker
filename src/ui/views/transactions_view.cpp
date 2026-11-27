#include "ui/views/transactions_view.hpp"

#include "core/formatters.hpp"
#include "imgui.h"
#include "ui/components.hpp"
#include "ui/theme.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>

namespace ui {
namespace {

bool ContainsInsensitive(const std::string& value, const std::string& query) {
    if (query.empty()) {
        return true;
    }
    return std::search(value.begin(), value.end(), query.begin(), query.end(),
        [](unsigned char left, unsigned char right) {
            return std::tolower(left) == std::tolower(right);
        }) != value.end();
}

bool SelectCsvPath(bool save, std::string& path) {
    std::array<char, MAX_PATH> filename{};
    OPENFILENAMEA dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = "CSV files (*.csv)\0*.csv\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = filename.data();
    dialog.nMaxFile = static_cast<DWORD>(filename.size());
    dialog.lpstrDefExt = "csv";
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
        (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    const BOOL selected = save
        ? GetSaveFileNameA(&dialog)
        : GetOpenFileNameA(&dialog);
    if (!selected) {
        return false;
    }
    path = filename.data();
    return true;
}

} // namespace

void RenderTransactionsView(
    core::AppState& state,
    data::DatabaseManager& db,
    const analytics::DashboardStats& stats,
    const theme::FontSet& fonts) {
    ImGui::PushFont(fonts.bold ? fonts.bold : ImGui::GetFont());
    ImGui::TextUnformatted("Transactions");
    ImGui::PopFont();
    ImGui::TextColored(theme::kTextMuted, "Search and manage your complete ledger.");
    ImGui::Spacing();

    if (ImGui::Button("+ Add transaction")) {
        state.editing_transaction_id.reset();
        state.open_add_expense_modal = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Export CSV")) {
        std::string path;
        if (SelectCsvPath(true, path)) {
            state.status_banner = db.ExportToCSV(path)
                ? "Transactions exported."
                : "CSV export failed.";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import CSV")) {
        std::string path;
        if (SelectCsvPath(false, path)) {
            const bool imported = db.ImportFromCSV(path);
            state.status_banner = imported
                ? "CSV imported; transactions were appended."
                : "CSV import failed. Check the file format and category IDs.";
            if (imported) {
                state.stats_dirty = true;
            }
        }
    }

    static std::array<char, 256> search_buffer{};
    static std::string previous_search;
    if (previous_search != state.search_query) {
        const std::size_t copy_size =
            std::min(state.search_query.size(), search_buffer.size() - 1);
        std::copy_n(state.search_query.data(), copy_size, search_buffer.data());
        search_buffer[copy_size] = '\0';
        previous_search = state.search_query;
    }
    ImGui::SetNextItemWidth(310.0f);
    if (ImGui::InputTextWithHint("##LedgerSearch", "Search merchant or note...",
            search_buffer.data(), search_buffer.size())) {
        state.search_query = search_buffer.data();
        state.stats_dirty = true;
    }

    ImGui::SameLine();
    const auto& categories = stats.categories;
    const core::Category* selected_category = nullptr;
    for (const core::Category& category : categories) {
        if (category.id == state.selected_category_id) {
            selected_category = &category;
            break;
        }
    }
    const char* category_label = selected_category
        ? selected_category->name.c_str()
        : "All categories";
    if (ImGui::BeginCombo("##LedgerCategory", category_label)) {
        if (ImGui::Selectable("All categories", state.selected_category_id == 0)) {
            state.selected_category_id = 0;
            state.stats_dirty = true;
        }
        for (const core::Category& category : categories) {
            if (ImGui::Selectable(category.name.c_str(),
                    state.selected_category_id == category.id)) {
                state.selected_category_id = category.id;
                state.stats_dirty = true;
            }
        }
        ImGui::EndCombo();
    }

    std::vector<const core::Transaction*> visible_transactions;
    visible_transactions.reserve(stats.all_transactions.size());
    for (const core::Transaction& transaction : stats.all_transactions) {
        const bool category_matches = state.selected_category_id == 0 ||
            transaction.category_id == state.selected_category_id;
        if (category_matches &&
            (ContainsInsensitive(transaction.merchant, state.search_query) ||
                ContainsInsensitive(transaction.note, state.search_query))) {
            visible_transactions.push_back(&transaction);
        }
    }
    ImGui::TextColored(theme::kTextMuted, "%zu transactions", visible_transactions.size());

    constexpr ImGuiTableFlags table_flags =
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##TransactionLedger", 6, table_flags, ImVec2(0.0f, 0.0f))) {
        ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 105.0f);
        ImGui::TableSetupColumn("Merchant", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed, 125.0f);
        ImGui::TableSetupColumn("Note", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 155.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (const core::Transaction* transaction : visible_transactions) {
            ImGui::PushID(static_cast<int>(transaction->id));
            ImGui::TableNextRow();
            if (transaction->is_anomaly) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                    ImGui::ColorConvertFloat4ToU32(ImVec4(
                        theme::kAccentCoral.x, theme::kAccentCoral.y,
                        theme::kAccentCoral.z, 0.11f)));
            }
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(transaction->date.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(transaction->merchant.c_str());
            ImGui::TableSetColumnIndex(2);
            const auto category = std::find_if(categories.begin(), categories.end(),
                [transaction](const core::Category& value) {
                    return value.id == transaction->category_id;
                });
            RenderCategoryPill(transaction->category_name,
                category == categories.end() ? "#38BDF8" : category->color_hex);
            if (transaction->is_anomaly) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, theme::kAccentCoral);
                ImGui::TextUnformatted("High Spend");
                ImGui::PopStyleColor();
            }
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(core::FormatCurrency(
                transaction->amount_cents, state.currency_symbol).c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(transaction->note.c_str());
            ImGui::TableSetColumnIndex(5);
            if (ImGui::SmallButton("Edit")) {
                state.editing_transaction_id = transaction->id;
                state.open_add_expense_modal = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete")) {
                if (db.DeleteTransaction(transaction->id)) {
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