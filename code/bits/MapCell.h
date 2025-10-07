#ifndef FFW_MAP_CELL_H
#define FFW_MAP_CELL_H

#include <cstdint>

#include <gf2/core/Color.h>
#include <gf2/core/Flags.h>
#include <gf2/core/TypeTraits.h>

namespace ffw {

  enum class MapCellBiome : uint8_t {
    None,

    Prairie,
    Desert,
    Forest,
    Moutain,

    Water,

    Underground,
    Building,
  };

  inline constexpr std::size_t MapCellBiomeCount = 8;

  enum class MapCellProperty : uint8_t {
    None      = 0x00,
    Visible   = 0x01,
    Explored  = 0x02,
  };

  using MapCellProperties = gf::Flags<MapCellProperty>;

  enum class MapCellDecoration : uint16_t {
    None,

    // walkable

    FloorDown,
    FloorUp,
    Herb,


    // not walkable and transparent

    Cactus = 0x4000,
    Tree,


    // not walkable and not transparent

    Cliff = 0xA000,
    Wall,
    Rock,

    // TODO: ores: gold, silver, coal, copper, iron (blocking and non-blocking version)
  };

  constexpr bool is_walkable(MapCellDecoration decoration)
  {
    return decoration < MapCellDecoration::Cactus;
  }

  constexpr bool is_transparent(MapCellDecoration decoration)
  {
    return decoration < MapCellDecoration::Cliff;
  }

  struct MapCell {
    MapCellBiome region = MapCellBiome::None;
    MapCellProperties properties = gf::None;
    MapCellDecoration decoration = MapCellDecoration::None;

    bool transparent() const
    {
      return is_transparent(decoration);
    }

    bool visible() const
    {
      return properties.test(MapCellProperty::Visible);
    }

    bool explored() const
    {
      return properties.test(MapCellProperty::Explored);
    }
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapCell, Archive>& cell)
  {
    return ar | cell.region | cell.properties | cell.decoration;
  }

}

template<>
struct gf::EnableBitmaskOperators<ffw::MapCellProperty> : std::true_type {
};

#endif // FFW_MAP_CELL_H
