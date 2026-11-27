#pragma once

#include "ui/theme.hpp"

#include <string_view>

namespace ui {

void RenderKpiCard(
    std::string_view title,
    std::string_view value,
    std::string_view subtitle,
    ImVec4 accent_color,
    const theme::FontSet& fonts);

void RenderCategoryPill(std::string_view name, std::string_view color_hex);

} // namespace ui
