#ifndef FFW_MAP_STATE_H
#define FFW_MAP_STATE_H

#include <gf2/core/Console.h>
#include <gf2/core/TypeTraits.h>

namespace ffw {

  struct MapState {
    gf::Console base;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapState, Archive>& state)
  {
    return ar | state.base;
  }

}

#endif // FFW_MAP_STATE_H
