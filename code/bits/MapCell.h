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

  // constant once generated
  struct MapTrait {
    double altitude;
    double moisture;
    gf::Color background;
    MapRegion region = MapRegion::Prairie;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapTrait, Archive>& trait)
  {
    return ar | trait.altitude | trait.moisture | trait.background | trait.region;
  }

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

  // may evolve during the game
  struct MapDetail {
    MapBlock block;
    MapDecoration decoration;
    float state;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapDetail, Archive>& detail)
  {
    return ar | detail.block | detail.decoration | detail.state;
  }

  struct MapCell {
    MapTrait trait;
    MapDetail detail;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapCell, Archive>& cell)
  {
    return ar | cell.trait | cell.detail;
  }

}

#endif // FFW_MAP_CELL_H
