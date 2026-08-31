#include "Date.h"

#include <cstdint>
#include <ctime>

#include <numbers>
#include <tuple>

#include <fmt/chrono.h>

namespace fw {

  namespace {
    constexpr double SolticeAmplitude = 3.17;
    constexpr double EquinoxOffset = 0.07;

    std::tm to_tm(const Date& date)
    {
      std::tm tm = {};
      tm.tm_sec = date.seconds;
      tm.tm_min = date.minutes;
      tm.tm_hour = date.hours;
      tm.tm_mday = date.day;
      tm.tm_mon = static_cast<MonthType>(date.month);
      tm.tm_wday = (static_cast<DayType>(date.weekday) + 1) % DaysInWeek;
      return tm;
    }

    uint32_t days_since_1st_jan(const MonthDay& month_day)
    {
      uint32_t days = 0;
      Month month = Month::Jan;

      while (month < month_day.month) {
        days += days_in_month(month);
        month = next_month(month);
      }

      days += month_day.day;
      assert(days <= DaysInYear);

      return days;
    }

    constexpr int32_t ExactNoonInSeconds = HoursInDay * MinutesInHour * SecondsInMinute / 2;

    int32_t daylight_seconds(uint32_t days)
    {
      static constexpr int32_t Equinox = days_in_month(Month::Jan) + days_in_month(Month::Feb) + 22; // 22 march

      return static_cast<int32_t>(ExactNoonInSeconds + SolticeAmplitude * MinutesInHour * SecondsInMinute * std::sin(2.0 * std::numbers::pi * (static_cast<double>(days) - static_cast<double>(Equinox) + EquinoxOffset) / DaysInYear));
    }

    int32_t compute_sunrise_in_seconds(int32_t daylight_in_seconds)
    {
      return ExactNoonInSeconds - daylight_in_seconds / 2;
    }

    int32_t compute_sunset_in_seconds(int32_t daylight_in_seconds)
    {
      return compute_sunrise_in_seconds(daylight_in_seconds) + daylight_in_seconds;
    }

    HourMinuteSeconds from_seconds(int32_t seconds)
    {
      HourMinuteSeconds hms = {};
      hms.seconds = seconds % SecondsInMinute;
      const int32_t minutes = seconds / SecondsInMinute;

      hms.minutes = minutes % MinutesInHour;
      const int32_t hours = minutes / MinutesInHour;

      hms.hours = static_cast<Hour>(hours % HoursInDay);
      return hms;
    }

  }

  /*
   * Date
   */

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

  void Date::add_seconds(Second duration_in_seconds)
  {
    seconds += duration_in_seconds;

    if (seconds >= SecondsInMinute) {
      minutes += seconds / SecondsInMinute;
      seconds %= SecondsInMinute;
    }

    assert(0 <= seconds && seconds < SecondsInMinute);

    if (minutes >= MinutesInHour) {
      hours += minutes / MinutesInHour;
      minutes %= MinutesInHour;
    }

    assert(0 <= minutes && minutes < MinutesInHour);

    while (hours >= HoursInDay) {
      ++day;
      weekday = next_weekday(weekday);

      if (day > days_in_month(month)) {
        day = 1;
        month = next_month(month);

        if (month == Month::Jan) {
          // it's a new year
          ++year;
        }
      }

      hours -= HoursInDay;
    }
  }

  Phase Date::phase() const
  {
    // compute the number of days from the beginning of the year

    const uint32_t days = days_since_1st_jan({ .month = month, .day = day });

    // compute the length of daytime

    const int32_t daylight_in_seconds = daylight_seconds(days);

    // compute milestones

    const int32_t sunrise = compute_sunrise_in_seconds(daylight_in_seconds);
    const int32_t dawn = sunrise - 30 * SecondsInMinute;

    const int32_t sunset = compute_sunset_in_seconds(daylight_in_seconds);
    const int32_t dusk = sunset + 30 * SecondsInMinute;

    const int32_t noon_begin = ExactNoonInSeconds - 30 * SecondsInMinute;
    const int32_t noon_end = ExactNoonInSeconds + 30 * SecondsInMinute;

    const int32_t past_seconds = hours * MinutesInHour * SecondsInMinute + minutes * SecondsInMinute + seconds;

    if (past_seconds < dawn) {
      return Phase::Night;
    }

    if (past_seconds < sunrise) {
      return Phase::Dawn;
    }

    if (past_seconds < noon_begin) {
      return Phase::Morning;
    }

    if (past_seconds < noon_end) {
      return Phase::Noon;
    }

    if (past_seconds < sunset) {
      return Phase::Afternoon;
    }

    if (past_seconds < dusk) {
      return Phase::Dusk;
    }

    return Phase::Night;
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
    assert(0 <= date.minutes && date.minutes < MinutesInHour);
    date.seconds = random->compute_uniform_integer(SecondsInMinute);
    assert(0 <= date.seconds && date.seconds < SecondsInMinute);

    return date;
  }

  bool operator<(const Date& lhs, const Date& rhs)
  {
    return std::tie(lhs.year, lhs.month, lhs.day, lhs.hours, lhs.minutes, lhs.seconds) < std::tie(rhs.year, rhs.month, rhs.day, rhs.hours, rhs.minutes, rhs.seconds);
  }

  bool operator==(const Date& lhs, const Date& rhs)
  {
    return std::tie(lhs.year, lhs.month, lhs.day, lhs.hours, lhs.minutes, lhs.seconds) == std::tie(rhs.year, rhs.month, rhs.day, rhs.hours, rhs.minutes, rhs.seconds);
  }

  /*
   * MonthDay
   */

  MonthDay generate_random_birthday(gf::Random* random)
  {
    const Month month = Month{ random->compute_uniform_integer(MonthsInYear) };
    Day day = random->compute_uniform_integer(days_in_month(month)); ++day;
    return { month, day };
  }

  /*
   * HourMinuteSeconds
   */

  std::string HourMinuteSeconds::to_string() const
  {
    return fmt::format("{:02}:{:02}:{:02}", hours, minutes, seconds);
  }

  HourMinuteSeconds compute_sunrise(const MonthDay& month_day)
  {
    const uint32_t days = days_since_1st_jan(month_day);
    const int32_t daylight_in_seconds = daylight_seconds(days);
    const int32_t sunrise = compute_sunrise_in_seconds(daylight_in_seconds);
    return from_seconds(sunrise);
  }

  HourMinuteSeconds compute_sunset(const MonthDay& month_day)
  {
    const uint32_t days = days_since_1st_jan(month_day);
    const int32_t daylight_in_seconds = daylight_seconds(days);
    const int32_t sunrise = compute_sunset_in_seconds(daylight_in_seconds);
    return from_seconds(sunrise);
  }

}
