#ifndef FFW_MAP_STATE_H
#define FFW_MAP_STATE_H

#include <cstdint>

#include <gf2/core/Array2D.h>
#include <gf2/core/TypeTraits.h>

#include "MapCell.h"

namespace ffw {

  enum class Building : uint8_t {
    Empty,
    None,
    // actual buildings
    Bank,
    Casino,
    Church,
    ClothShop,
    FoodShop,
    Hotel,
    House1,
    House2,
    House3,
    MarshalOffice,
    Restaurant,
    Saloon,
    WeaponShop,
  };

  constexpr std::size_t TownsCount = 5;
  constexpr int32_t TownsBlockSize = 6;
  constexpr int32_t BuildingSize = 11;
  constexpr int32_t StreetSize = 3;

  constexpr int32_t TownRadius = (TownsBlockSize * BuildingSize + (TownsBlockSize - 1) * StreetSize - 1) / 2;
  constexpr int32_t TownDiameter = 2 * TownRadius + 1;

  struct TownState {
    gf::Vec2I position;
    std::array<std::array<Building, TownsBlockSize>, TownsBlockSize> buildings = {};
    uint8_t horizontal_street = 1; // [1, TownsBlockSize - 1]
    uint8_t vertical_street = 1; // [1, TownsBlockSize - 1]
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<TownState, Archive>& state)
  {
    return ar | state.position | state.buildings | state.horizontal_street | state.vertical_street;
  }

  struct MapState {
    gf::Array2D<MapCell> cells;
    std::array<TownState, TownsCount> towns;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapState, Archive>& state)
  {
    return ar | state.cells | state.towns;
  }

}

#endif // FFW_MAP_STATE_H
