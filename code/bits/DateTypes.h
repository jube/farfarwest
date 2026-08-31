#ifndef FW_DATE_TYPES_H
#define FW_DATE_TYPES_H

#include <cstdint>

namespace fw {

  using Second = int32_t;
  using Minute = int32_t;
  using Hour = int32_t;
  using Day = uint8_t;

  using DayType = Day;
  using MonthType = uint8_t;

  using Year = int8_t;

}

#endif // FW_DATE_TYPES_H
