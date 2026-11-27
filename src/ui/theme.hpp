#pragma once

#include "imgui.h"
#include <string_view>

namespace ui::theme {

// Obsidian Slate Color Palette
inline constexpr ImVec4 kBgCanvas      = ImVec4(0.059f, 0.067f, 0.090f, 1.00f); // #0F1117
inline constexpr ImVec4 kBgSidebar     = ImVec4(0.071f, 0.082f, 0.114f, 1.00f); // #12151D
inline constexpr ImVec4 kBgCard        = ImVec4(0.086f, 0.106f, 0.149f, 1.00f); // #161B26
inline constexpr ImVec4 kBgElevated    = ImVec4(0.118f, 0.145f, 0.204f, 1.00f); // #1E2534
inline constexpr ImVec4 kBorderSubtle  = ImVec4(0.149f, 0.180f, 0.239f, 1.00f); // #262E3D

inline constexpr ImVec4 kTextPrimary   = ImVec4(0.945f, 0.961f, 0.976f, 1.00f); // #F1F5F9
inline constexpr ImVec4 kTextMuted     = ImVec4(0.580f, 0.639f, 0.722f, 1.00f); // #94A3B8

inline constexpr ImVec4 kAccentPrimary = ImVec4(0.220f, 0.741f, 0.973f, 1.00f); // #38BDF8 (Sky)
inline constexpr ImVec4 kAccentEmerald = ImVec4(0.063f, 0.725f, 0.506f, 1.00f); // #10B981 (Positive)
inline constexpr ImVec4 kAccentCoral   = ImVec4(0.957f, 0.247f, 0.369f, 1.00f); // #F43F5E (Overspend)
inline constexpr ImVec4 kAccentAmber   = ImVec4(0.961f, 0.620f, 0.043f, 1.00f); // #F59E0B (Warning)

struct FontSet {
    ImFont* regular = nullptr;
    ImFont* bold = nullptr;
    ImFont* kpi_hero = nullptr;
};

// Applies custom ImGui & ImPlot styles, rounding, spacing, and dark palette
void ApplyObsidianSlateTheme(float dpi_scale = 1.0f);

// Loads Inter fonts from assets/fonts/ if available, falling back to default font
FontSet LoadAppFonts(float dpi_scale = 1.0f);

// Converts "#RRGGBB" hex strings into ImVec4
ImVec4 HexToImVec4(std::string_view hex, float alpha = 1.0f);

} // namespace ui::theme