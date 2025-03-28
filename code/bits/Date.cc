#include "Date.h"
#include <cstdint>

namespace ffw {

  namespace {
    constexpr uint16_t SecondsInMinute = 60;
    constexpr uint16_t MinutesInHour = 60;
    constexpr uint8_t HoursInDay = 24;
    constexpr uint8_t DaysInWeek = 7;
    constexpr uint8_t MonthsInYear = 12;

    constexpr uint8_t DaysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  }


  Date& Date::operator+=(uint16_t duration_in_seconds)
  {
    seconds += duration_in_seconds;

    if (seconds >= SecondsInMinute) {
      minutes += seconds / SecondsInMinute;
      seconds %= SecondsInMinute;
    }

    if (minutes >= MinutesInHour) {
      hours += minutes / MinutesInHour;
      minutes %= MinutesInHour;
    }

    while (hours >= HoursInDay) {
      ++day;
      weekday = WeekDay{uint8_t((uint8_t(weekday) + 1) % DaysInWeek)};

      if (day > DaysInMonth[uint8_t(month)]) {
        day = 1;
        month = Month{uint8_t((uint8_t(month) + 1) % MonthsInYear)};
      }

      hours -= HoursInDay;
    }

    return *this;
  }


}
