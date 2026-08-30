#ifndef FW_DATE_TYPES_H
#define FW_DATE_TYPES_H

#include <cstdint>

namespace fw {

  using Second = int16_t;
  using Minute = int16_t;
  using Hour = int16_t;
  using Day = uint8_t;

  using DayType = Day;
  using MonthType = uint8_t;

  using Year = int8_t;

}

#endif // FW_DATE_TYPES_H
