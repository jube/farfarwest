#ifndef FW_NETWORK_STATE_H
#define FW_NETWORK_STATE_H

#include <cstdint>

#include <vector>

#include <gf2/core/TypeTraits.h>
#include <gf2/core/Vec2.h>

#include "DateTypes.h"

namespace fw {

  constexpr std::size_t TrainLength = 12;

  struct StationState {
    uint32_t index;
    Second stop_time;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<StationState, Archive>& state)
  {
    return ar | state.index | state.stop_time;
  }

  struct NetworkState {
    std::vector<gf::Vec2I> railway;
    std::vector<StationState> stations;

    std::vector<gf::Vec2I> roads;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<NetworkState, Archive>& state)
  {
    return ar | state.railway | state.stations | state.roads;
  }

}

#endif // FW_NETWORK_STATE_H
