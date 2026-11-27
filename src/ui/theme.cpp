#include "ui/theme.hpp"
#include "implot.h"

#include <charconv>
#include <filesystem>

namespace ui::theme {

void ApplyObsidianSlateTheme(float dpi_scale) {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 10.0f;
    style.ChildRounding     = 10.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;

    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(16.0f, 16.0f);
    style.FramePadding      = ImVec2(12.0f, 8.0f);
    style.ItemSpacing       = ImVec2(12.0f, 12.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize     = 10.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                  = kTextPrimary;
    colors[ImGuiCol_TextDisabled]          = kTextMuted;
    colors[ImGuiCol_WindowBg]              = kBgCanvas;
    colors[ImGuiCol_ChildBg]               = kBgCard;
    colors[ImGuiCol_PopupBg]               = kBgElevated;
    colors[ImGuiCol_Border]                = kBorderSubtle;
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    colors[ImGuiCol_FrameBg]               = kBgElevated;
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.157f, 0.192f, 0.267f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.196f, 0.239f, 0.329f, 1.00f);

    colors[ImGuiCol_TitleBg]               = kBgSidebar;
    colors[ImGuiCol_TitleBgActive]         = kBgSidebar;
    colors[ImGuiCol_MenuBarBg]             = kBgSidebar;

    colors[ImGuiCol_ScrollbarBg]           = kBgCanvas;
    colors[ImGuiCol_ScrollbarGrab]         = kBorderSubtle;
    colors[ImGuiCol_ScrollbarGrabHovered]  = kTextMuted;
    colors[ImGuiCol_ScrollbarGrabActive]   = kAccentPrimary;

    colors[ImGuiCol_CheckMark]             = kAccentPrimary;
    colors[ImGuiCol_SliderGrab]            = kAccentPrimary;
    colors[ImGuiCol_SliderGrabActive]      = kAccentEmerald;

    colors[ImGuiCol_Button]                = kBgElevated;
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.165f, 0.204f, 0.282f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.220f, 0.741f, 0.973f, 0.30f);

    colors[ImGuiCol_Header]                = ImVec4(0.220f, 0.741f, 0.973f, 0.18f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.220f, 0.741f, 0.973f, 0.28f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.220f, 0.741f, 0.973f, 0.38f);

    colors[ImGuiCol_Separator]             = kBorderSubtle;
    colors[ImGuiCol_TableHeaderBg]         = kBgElevated;
    colors[ImGuiCol_TableBorderStrong]     = kBorderSubtle;
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.149f, 0.180f, 0.239f, 0.50f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

    style.ScaleAllSizes(dpi_scale);

    // Configure ImPlot styling to match Obsidian Slate
    ImPlotStyle& plot_style = ImPlot::GetStyle();
    
    plot_style.PlotPadding      = ImVec2(12.0f, 12.0f);
    plot_style.LabelPadding     = ImVec2(6.0f, 6.0f);
    plot_style.LegendPadding    = ImVec2(10.0f, 8.0f);
    

    ImVec4* pcolors = plot_style.Colors;
    pcolors[ImPlotCol_FrameBg]       = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    pcolors[ImPlotCol_PlotBg]        = ImVec4(0.047f, 0.055f, 0.075f, 0.65f);
    pcolors[ImPlotCol_PlotBorder]    = kBorderSubtle;
    pcolors[ImPlotCol_LegendBg]      = kBgElevated;
    pcolors[ImPlotCol_LegendBorder]  = kBorderSubtle;
    pcolors[ImPlotCol_LegendText]    = kTextPrimary;
    pcolors[ImPlotCol_TitleText]     = kTextPrimary;
    pcolors[ImPlotCol_InlayText]     = kTextPrimary;
    pcolors[ImPlotCol_AxisText]      = kTextMuted;
    pcolors[ImPlotCol_AxisGrid]      = ImVec4(0.149f, 0.180f, 0.239f, 0.65f);
    pcolors[ImPlotCol_AxisTick]      = kBorderSubtle;
    pcolors[ImPlotCol_AxisBg]        = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    pcolors[ImPlotCol_AxisBgHovered] = ImVec4(0.220f, 0.741f, 0.973f, 0.10f);
    pcolors[ImPlotCol_AxisBgActive]  = ImVec4(0.220f, 0.741f, 0.973f, 0.20f);

    // Register a modern categorical fintech colormap
    static const ImVec4 kFintechPalette[] = {
        kAccentPrimary,                          // Sky Blue
        kAccentEmerald,                          // Emerald
        ImVec4(0.545f, 0.361f, 0.965f, 1.00f),   // Violet (#8B5CF6)
        kAccentAmber,                            // Amber
        kAccentCoral,                            // Coral
        ImVec4(0.078f, 0.722f, 0.651f, 1.00f),   // Teal (#14B8A6)
        ImVec4(0.925f, 0.282f, 0.600f, 1.00f),   // Pink (#EC4899)
        ImVec4(0.388f, 0.400f, 0.945f, 1.00f)    // Indigo (#6366F1)
    };
    plot_style.Colormap = ImPlot::AddColormap("ObsidianFinTech", kFintechPalette, 8);
}

FontSet LoadAppFonts(float dpi_scale) {
    ImGuiIO& io = ImGui::GetIO();
    FontSet fonts{};

#if IMGUI_VERSION_NUM >= 19200
    ImGui::GetStyle().FontScaleDpi = dpi_scale;
    const float base_size = 16.0f;
    const float hero_size = 28.0f;
#else
    const float base_size = 16.0f * dpi_scale;
    const float hero_size = 28.0f * dpi_scale;
#endif

    const std::filesystem::path regular_path = "assets/fonts/Inter-Regular.ttf";
    const std::filesystem::path bold_path    = "assets/fonts/Inter-Bold.ttf";

    if (std::filesystem::exists(regular_path)) {
        fonts.regular = io.Fonts->AddFontFromFileTTF(regular_path.string().c_str(), base_size);
    } else {
        ImFontConfig cfg;
        cfg.SizePixels = base_size;
        fonts.regular = io.Fonts->AddFontDefault(&cfg);
    }

    if (std::filesystem::exists(bold_path)) {
        fonts.bold     = io.Fonts->AddFontFromFileTTF(bold_path.string().c_str(), base_size);
        fonts.kpi_hero = io.Fonts->AddFontFromFileTTF(bold_path.string().c_str(), hero_size);
    } else {
        fonts.bold     = fonts.regular;
        fonts.kpi_hero = fonts.regular;
    }

    return fonts;
}

ImVec4 HexToImVec4(std::string_view hex, float alpha) {
    if (!hex.empty() && hex.front() == '#') {
        hex.remove_prefix(1);
    }
    if (hex.size() != 6) {
        return kAccentPrimary;
    }

    unsigned int rgb = 0;
    auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), rgb, 16);
    if (ec != std::errc{}) {
        return kAccentPrimary;
    }

    const float r = static_cast<float>((rgb >> 16) & 0xFF) / 255.0f;
    const float g = static_cast<float>((rgb >> 8) & 0xFF) / 255.0f;
    const float b = static_cast<float>(rgb & 0xFF) / 255.0f;
    return ImVec4(r, g, b, alpha);
}

} // namespace ui::theme