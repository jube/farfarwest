#ifndef FFW_TIMES_H
#define FFW_TIMES_H

#include <cstdint>

namespace ffw {

  constexpr uint16_t TrainTime = 5;

  constexpr uint16_t StraightWalkTime = 15;
  constexpr uint16_t DiagonalWalkTime = 21; // = 15 * sqrt(2)
  constexpr uint16_t HeroIdleTime = 60;

  constexpr uint16_t MountTime = 10;
  constexpr uint16_t DismountTime = 10;

  constexpr uint16_t GrazeTime = 100;
  constexpr uint16_t IdleTime = 100;

}

#endif // FFW_TIMES_H
