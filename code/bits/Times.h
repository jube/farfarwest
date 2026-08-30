#ifndef FW_TIMES_H
#define FW_TIMES_H

#include "DateTypes.h"

namespace fw {

  constexpr Second TrainTime = 5;

  constexpr Second StraightWalkTime = 15;
  constexpr Second DiagonalWalkTime = 21; // = 15 * sqrt(2)
  constexpr Second HeroIdleTime = 60;

  constexpr Second MountTime = 10;
  constexpr Second DismountTime = 10;

  constexpr Second GrazeTime = 100;
  constexpr Second IdleTime = 100;
  constexpr Second WanderTime = 50;
  constexpr Second WanderIdleTime = 25;

}

#endif // FW_TIMES_H
