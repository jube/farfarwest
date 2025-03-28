#ifndef FFW_DATE_H
#define FFW_DATE_H

#include <cstdint>

#include <gf2/core/TypeTraits.h>

namespace ffw {

  enum class WeekDay : uint8_t {
    Mon,
    Tue,
    Wed,
    Thu,
    Fri,
    Sat,
    Sun,
  };

  enum class Month : uint8_t {
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

  struct Date {
    Month month;
    uint8_t day;
    WeekDay weekday;
    uint8_t hours;
    uint16_t minutes;
    uint16_t seconds;

    Date& operator+=(uint16_t duration_in_seconds);
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<Date, Archive>& date)
  {
    return ar | date.month | date.day | date.weekday | date.hours | date.minutes | date.seconds;
  }

}

#endif // FFW_DATE_H
