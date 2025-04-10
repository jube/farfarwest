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

  enum class MapBlock : uint8_t {
    None,
    Cactus,
    Cliff,
    Tree,
  };

  enum MapDecoration : uint8_t {
    None,
    Herb,
  };

  struct MapCell {
    MapRegion region = MapRegion::Prairie;
    MapBlock block = MapBlock::None;
    MapDecoration decoration = MapDecoration::None;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapCell, Archive>& cell)
  {
    return ar | cell.region | cell.block | cell.decoration;
  }

}

#endif // FFW_MAP_CELL_H
