#ifndef FFW_DATE_H
#define FFW_DATE_H

#include <cstdint>

#include <string>

#include <gf2/core/Random.h>
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
    uint8_t year;
    Month month;
    uint8_t day;
    WeekDay weekday;
    uint8_t hours;
    uint16_t minutes;
    uint16_t seconds;

    std::string to_string() const;
    void add_seconds(uint16_t duration_in_seconds);

    static Date generate_random(gf::Random* random);
  };

  inline Date operator+(const Date& date, uint16_t duration_in_seconds)
  {
    Date future(date);
    future.add_seconds(duration_in_seconds);
    return future;
  }

  bool operator<(const Date& lhs, const Date& rhs);
  bool operator==(const Date& lhs, const Date& rhs);

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<Date, Archive>& date)
  {
    return ar | date.year | date.month | date.day | date.weekday | date.hours | date.minutes | date.seconds;
  }

}

#endif // FFW_DATE_H
