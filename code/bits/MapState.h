#ifndef FFW_MAP_STATE_H
#define FFW_MAP_STATE_H

#include <cstdint>

#include <gf2/core/Array2D.h>
#include <gf2/core/TypeTraits.h>

#include "MapCell.h"

namespace ffw {

  constexpr uint32_t TrainSize = 8;

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
    uint32_t index;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<TrainState, Archive>& state)
  {
    return ar | state.index;
  }

  struct NetworkState {
    std::vector<gf::Vec2I> railway;
    std::vector<StationState> stations;
    std::vector<TrainState> trains;

    uint32_t next_position(uint32_t current, uint32_t advance = 1) const;
    uint32_t prev_position(uint32_t current, uint32_t advance = 1) const;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<NetworkState, Archive>& state)
  {
    return ar | state.railway | state.stations | state.trains;
  }

  struct MapState {
    gf::Array2D<MapCell> cells;
    NetworkState network;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapState, Archive>& state)
  {
    return ar | state.cells | state.network;
  }

}

#endif // FFW_MAP_STATE_H
