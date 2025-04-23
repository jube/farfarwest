#ifndef FFW_MAP_STATE_H
#define FFW_MAP_STATE_H

#include <cstdint>

#include <gf2/core/Array2D.h>
#include <gf2/core/TypeTraits.h>

#include "MapCell.h"

namespace ffw {

  struct MapState {
    gf::Array2D<MapCell> cells;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapState, Archive>& state)
  {
    return ar | state.cells;
  }

}

#endif // FFW_MAP_STATE_H
