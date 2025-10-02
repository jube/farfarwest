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
    Dirt,
  };

  enum class MapDecoration : uint8_t {
    None,

    // non-blocking

    FloorDown,
    FloorUp,
    Herb,


    // not walkable and transparent

    Cactus = 0x40,
    Tree,


    // not walkable and not transparent

    Cliff = 0xA0,
    Wall,

    // TODO: ores: gold, silver, coal, copper, iron (blocking and non-blocking version)
  };

  constexpr bool is_walkable(MapDecoration decoration)
  {
    return decoration < MapDecoration::Cactus;
  }

  constexpr bool is_transparent(MapDecoration decoration)
  {
    return decoration < MapDecoration::Cliff;
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
    MapUnderground type = MapUnderground::Dirt;
    MapDecoration decoration = MapDecoration::Wall;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapUndergroundCell, Archive>& cell)
  {
    return ar | cell.type | cell.decoration;
  }

}

#endif // FFW_MAP_CELL_H
