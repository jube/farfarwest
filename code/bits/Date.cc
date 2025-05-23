#include "Date.h"

#include <cstdint>
#include <ctime>

#include <tuple>

#include <fmt/chrono.h>

namespace ffw {

  namespace {
    constexpr uint16_t SecondsInMinute = 60;
    constexpr uint16_t MinutesInHour = 60;
    constexpr uint8_t HoursInDay = 24;
    constexpr uint8_t DaysInWeek = 7;
    constexpr uint8_t MonthsInYear = 12;

    constexpr int DaysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    int days_in_month(Month month)
    {
      return DaysInMonth[uint8_t(month)];
    }

    std::tm to_tm(const Date& date)
    {
      std::tm tm = {};
      tm.tm_sec = date.seconds;
      tm.tm_min = date.minutes;
      tm.tm_hour = date.hours;
      tm.tm_mday = date.day;
      tm.tm_mon = uint8_t(date.month);
      tm.tm_wday = uint8_t(date.weekday);
      return tm;
    }

  }

  std::string Date::to_string() const
  {
    auto tm = to_tm(*this);
    return fmt::format("{:%a %d %b %r}", tm);
  }

  std::string Date::to_string_hours_minutes() const
  {
    auto tm = to_tm(*this);
    return fmt::format("{:%R %p}", tm);
  }

  void Date::add_seconds(uint16_t duration_in_seconds)
  {
    seconds += duration_in_seconds;

    if (seconds >= SecondsInMinute) {
      minutes += seconds / SecondsInMinute;
      seconds %= SecondsInMinute;
    }

    if (minutes >= MinutesInHour) {
      hours += static_cast<uint8_t>(minutes / MinutesInHour);
      minutes %= MinutesInHour;
    }

    while (hours >= HoursInDay) {
      ++day;
      weekday = WeekDay{uint8_t((uint8_t(weekday) + 1) % DaysInWeek)};

      if (day > days_in_month(month)) {
        day = 1;
        month = Month{uint8_t((uint8_t(month) + 1) % MonthsInYear)};

        if (month == Month::Jan) {
          // it's a new year
          ++year;
        }
      }

      hours -= HoursInDay;
    }
  }

  Date Date::generate_random(gf::Random* random)
  {
    Date date = {};

    date.year = 0; // not used publicly so it's ok to set it to 0
    date.month = Month{ random->compute_uniform_integer(MonthsInYear) };
    date.day = random->compute_uniform_integer(days_in_month(date.month)); ++date.day;

    date.weekday = WeekDay { random->compute_uniform_integer(DaysInWeek) };

    date.hours = 12;
    date.minutes = random->compute_uniform_integer(MinutesInHour);
    date.seconds = random->compute_uniform_integer(SecondsInMinute);

    return date;
  }

  MonthDay generate_random_birthday(gf::Random* random)
  {
    const Month month = Month{ random->compute_uniform_integer(MonthsInYear) };
    uint8_t day = random->compute_uniform_integer(days_in_month(month)); ++day;
    return { month, day };
  }

  bool operator<(const Date& lhs, const Date& rhs)
  {
    return std::tie(lhs.year, lhs.month, lhs.day, lhs.hours, lhs.minutes, lhs.seconds) < std::tie(rhs.year, rhs.month, rhs.day, rhs.hours, rhs.minutes, rhs.seconds);
  }

  bool operator==(const Date& lhs, const Date& rhs)
  {
    return std::tie(lhs.year, lhs.month, lhs.day, lhs.hours, lhs.minutes, lhs.seconds) == std::tie(rhs.year, rhs.month, rhs.day, rhs.hours, rhs.minutes, rhs.seconds);
  }

}
