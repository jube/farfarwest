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


  struct MapCell {
    MapTrait trait;

  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapCell, Archive>& cell)
  {
    return ar | cell.trait;
  }

}

#endif // FFW_MAP_CELL_H
