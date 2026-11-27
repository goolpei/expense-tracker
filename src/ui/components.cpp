#include "ui/components.hpp"

#include "imgui.h"

#include <string>

namespace ui {

void RenderKpiCard(
    std::string_view title,
    std::string_view value,
    std::string_view subtitle,
    ImVec4 accent_color,
    const theme::FontSet& fonts) {
    const std::string title_text(title);
    ImGui::BeginChild(
        (title_text + "##KpiCard").c_str(),
        ImVec2(0.0f, 124.0f),
        ImGuiChildFlags_Borders);
    ImGui::TextColored(theme::kTextMuted, "%.*s",
        static_cast<int>(title.size()), title.data());
    ImGui::Spacing();
    ImGui::PushFont(fonts.kpi_hero ? fonts.kpi_hero : ImGui::GetFont());
    ImGui::TextColored(accent_color, "%.*s",
        static_cast<int>(value.size()), value.data());
    ImGui::PopFont();
    ImGui::TextColored(theme::kTextMuted, "%.*s",
        static_cast<int>(subtitle.size()), subtitle.data());
    ImGui::EndChild();
}

void RenderCategoryPill(std::string_view name, std::string_view color_hex) {
    const ImVec4 color = theme::HexToImVec4(color_hex, 0.18f);
    const std::string text(name);
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::HexToImVec4(color_hex, 0.65f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 3.0f));
    ImGui::Button(text.c_str(), ImVec2(0.0f, 0.0f));
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}

} // namespace ui