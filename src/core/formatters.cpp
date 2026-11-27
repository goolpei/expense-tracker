#include "core/formatters.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace core {
namespace {

struct CalendarDate {
    int year = 1970;
    int month = 1;
    int day = 1;
};

CalendarDate Today() {
    std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    if (!local) {
        return {};
    }
    return {local->tm_year + 1900, local->tm_mon + 1, local->tm_mday};
}

bool IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int DaysInMonth(int year, int month) {
    static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return month == 2 && IsLeapYear(year) ? 29 : kDays[month - 1];
}

std::string FormatDate(const CalendarDate& date) {
    std::ostringstream out;
    out << std::setw(4) << std::setfill('0') << date.year << '-'
        << std::setw(2) << std::setfill('0') << date.month << '-'
        << std::setw(2) << std::setfill('0') << date.day;
    return out.str();
}

CalendarDate AddDays(CalendarDate date, int days) {
    while (days > 0) {
        int remaining = DaysInMonth(date.year, date.month) - date.day + 1;
        if (days < remaining) {
            date.day += days;
            return date;
        }
        days -= remaining;
        date.day = 1;
        if (++date.month > 12) {
            date.month = 1;
            ++date.year;
        }
    }
    while (days < 0) {
        if (--date.day >= 1) {
            ++days;
        } else if (--date.month >= 1) {
            date.day = DaysInMonth(date.year, date.month);
            ++days;
        } else {
            date.year--;
            date.month = 12;
            date.day = 31;
            ++days;
        }
    }
    return date;
}

CalendarDate AddMonths(CalendarDate date, int months) {
    int total = date.year * 12 + (date.month - 1) + months;
    date.year = total / 12;
    date.month = total % 12 + 1;
    date.day = std::min(date.day, DaysInMonth(date.year, date.month));
    return date;
}

} // namespace

std::string FormatCurrency(int64_t cents, std::string_view symbol) {
    const bool negative = cents < 0;
    uint64_t magnitude = negative
        ? static_cast<uint64_t>(-(cents + 1)) + 1U
        : static_cast<uint64_t>(cents);

    const uint64_t dollars = magnitude / 100U;
    const uint64_t remainder = magnitude % 100U;

    std::string dollars_text = std::to_string(dollars);
    for (std::size_t pos = dollars_text.size(); pos > 3;) {
        pos -= 3;
        dollars_text.insert(pos, ",");
    }

    std::ostringstream out;
    if (negative) {
        out << '-';
    }
    out << symbol << dollars_text << '.'
        << std::setw(2) << std::setfill('0') << remainder;
    return out.str();
}

std::string GetCurrentDateISO() {
    return FormatDate(Today());
}

std::string GetCurrentMonthISO() {
    CalendarDate today = Today();
    std::ostringstream out;
    out << std::setw(4) << std::setfill('0') << today.year << '-'
        << std::setw(2) << std::setfill('0') << today.month;
    return out.str();
}

DateRangeBounds GetDateRangeBounds(DateRangePreset preset) {
    const CalendarDate today = Today();

    switch (preset) {
    case DateRangePreset::ThisMonth:
        return {FormatDate({today.year, today.month, 1}),
                FormatDate({today.year, today.month, DaysInMonth(today.year, today.month)})};
    case DateRangePreset::Last30Days:
        return {FormatDate(AddDays(today, -29)), FormatDate(today)};
    case DateRangePreset::Last6Months:
        return {FormatDate({AddMonths(today, -5).year, AddMonths(today, -5).month, 1}),
                FormatDate(today)};
    case DateRangePreset::YearToDate:
        return {FormatDate({today.year, 1, 1}), FormatDate(today)};
    case DateRangePreset::AllTime:
        return {"1970-01-01", FormatDate(today)};
    }

    return {FormatDate(today), FormatDate(today)};
}

} // namespace core