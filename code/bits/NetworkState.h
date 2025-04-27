#ifndef FFW_NETWORK_STATE_H
#define FFW_NETWORK_STATE_H

#include <cstdint>

#include <vector>

#include <gf2/core/TypeTraits.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  constexpr std::size_t TrainLength = 11;

  struct StationState {
    uint32_t index;
    uint16_t stop_time;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<StationState, Archive>& state)
  {
    return ar | state.index | state.stop_time;
  }

  struct TrainState {
    uint32_t railway_index;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<TrainState, Archive>& state)
  {
    return ar | state.railway_index;
  }

  struct NetworkState {
    std::vector<gf::Vec2I> railway;
    std::vector<StationState> stations;
    std::vector<TrainState> trains;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<NetworkState, Archive>& state)
  {
    return ar | state.railway | state.stations | state.trains;
  }

}

#endif // FFW_NETWORK_STATE_H
