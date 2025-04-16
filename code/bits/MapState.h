#ifndef FFW_MAP_STATE_H
#define FFW_MAP_STATE_H

#include <gf2/core/Array2D.h>
#include <gf2/core/TypeTraits.h>

#include "MapCell.h"

namespace ffw {

  struct NetworkState {
    std::vector<gf::Vec2I> railway;
    std::vector<gf::Vec2I> stations;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<NetworkState, Archive>& state)
  {
    return ar | state.railway | state.stations;
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
