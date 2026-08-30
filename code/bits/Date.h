#ifndef FW_DATE_H
#define FW_DATE_H

#include <cstdint>

#include <numeric>
#include <string>

#include <gf2/core/Random.h>
#include <gf2/core/TypeTraits.h>

#include "DateTypes.h"

namespace fw {

  constexpr Second SecondsInMinute = 60;
  constexpr Minute MinutesInHour = 60;
  constexpr Hour HoursInDay = 24;
  constexpr Day DaysInWeek = 7;
  constexpr MonthType MonthsInYear = 12;

  constexpr DayType DaysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  constexpr uint32_t DaysInYear = std::accumulate(std::begin(DaysInMonth), std::end(DaysInMonth), 0u);

  enum class WeekDay : DayType {
    Mon,
    Tue,
    Wed,
    Thu,
    Fri,
    Sat,
    Sun,
  };

  constexpr WeekDay next_weekday(WeekDay weekday)
  {
    return WeekDay{static_cast<DayType>((static_cast<DayType>(weekday) + 1) % DaysInWeek)};
  }

  enum class Month : MonthType {
    Jan,
    Feb,
    Mar,
    Apr,
    Jun,
    Jul,
    Aug,
    Sep,
    Oct,
    Nov,
    Dec,
  };

  constexpr DayType days_in_month(Month month)
  {
    return DaysInMonth[static_cast<MonthType>(month)];
  }

  constexpr Month next_month(Month month)
  {
    return Month{static_cast<MonthType>((static_cast<MonthType>(month) + 1) % MonthsInYear)};
  }

  enum class Phase {
    Dawn,
    Morning,
    Noon,
    Afternoon,
    Dusk,
    Night,
  };

  struct Date {
    Year year;
    Month month;
    Day day;
    WeekDay weekday;
    Hour hours;
    Minute minutes;
    Second seconds;

    std::string to_string() const;
    std::string to_string_hours_minutes() const;

    void add_seconds(Second duration_in_seconds);

    Phase phase() const;

    static Date generate_random(gf::Random* random);
  };

  bool operator<(const Date& lhs, const Date& rhs);
  bool operator==(const Date& lhs, const Date& rhs);

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<Date, Archive>& date)
  {
    return ar | date.year | date.month | date.day | date.weekday | date.hours | date.minutes | date.seconds;
  }

  struct MonthDay {
    Month month;
    Day day;
  };

  MonthDay generate_random_birthday(gf::Random* random);

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MonthDay, Archive>& month_day)
  {
    return ar | month_day.month | month_day.day;
  }

  struct HourMinuteSeconds {
    Hour hours;
    Minute minutes;
    Second seconds;

    std::string to_string() const;
  };

  HourMinuteSeconds compute_sunrise(const MonthDay& month_day);
  HourMinuteSeconds compute_sunset(const MonthDay& month_day);

}

#endif // FW_DATE_H
