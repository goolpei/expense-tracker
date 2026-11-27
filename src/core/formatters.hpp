#pragma once

#include "core/models.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace core {

struct DateRangeBounds {
    std::string start_date;
    std::string end_date;
};

std::string FormatCurrency(int64_t cents, std::string_view symbol = "$");
std::string GetCurrentDateISO();
std::string GetCurrentMonthISO();
DateRangeBounds GetDateRangeBounds(DateRangePreset preset);

} // namespace core
