#ifndef FFW_MAP_CELL_H
#define FFW_MAP_CELL_H

#include <cstdint>

#include <gf2/core/Color.h>
#include <gf2/core/TypeTraits.h>

namespace ffw {

  enum class MapRegion : uint8_t {
    Prairie,
    Desert,
    Forest,
    Moutain,
  };

  enum class MapUnderground :uint8_t {
    Rock,
  };

  enum class MapDecoration : uint8_t {
    None,

    // non-blocking

    Herb,

    // blocking

    Cactus = 0x80,
    Cliff,
    Tree,
  };

  constexpr bool is_blocking(MapDecoration decoration)
  {
    return decoration >= MapDecoration::Cactus;
  }

  struct MapCell {
    MapRegion region = MapRegion::Prairie;
    MapDecoration decoration = MapDecoration::None;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapCell, Archive>& cell)
  {
    return ar | cell.region | cell.decoration;
  }

  struct MapUndergroundCell {
    MapUnderground type = MapUnderground::Rock;
    MapDecoration decoration = MapDecoration::None;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapUndergroundCell, Archive>& cell)
  {
    return ar | cell.type | cell.decoration;
  }

}

#endif // FFW_MAP_CELL_H
