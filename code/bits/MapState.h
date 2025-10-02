#ifndef FFW_MAP_STATE_H
#define FFW_MAP_STATE_H

#include <cstdint>

#include <gf2/core/Array2D.h>
#include <gf2/core/Direction.h>
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
    School,
    WeaponShop,
  };

  constexpr std::size_t TownsCount = 5;
  constexpr int32_t TownsBlockSize = 6;
  constexpr int32_t TownBuildingSize = 11;
  constexpr int32_t StreetSize = 3;

  constexpr int32_t TownRadius = (TownsBlockSize * TownBuildingSize + (TownsBlockSize - 1) * StreetSize - 1) / 2;
  constexpr int32_t TownDiameter = 2 * TownRadius + 1;

  struct TownState {
    gf::Vec2I position;
    std::array<std::array<Building, TownsBlockSize>, TownsBlockSize> buildings = {};
    uint8_t horizontal_street = 1; // [1, TownsBlockSize - 1]
    uint8_t vertical_street = 1; // [1, TownsBlockSize - 1]

    Building& operator()(gf::Vec2I building_position)
    {
      return buildings[building_position.y][building_position.x];
    }

    const Building& operator()(gf::Vec2I building_position) const
    {
      return buildings[building_position.y][building_position.x];
    }

  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<TownState, Archive>& state)
  {
    return ar | state.position | state.buildings | state.horizontal_street | state.vertical_street;
  }

  constexpr std::size_t LocalityPerTown = 5;
  constexpr std::size_t LocalityCount = TownsCount * LocalityPerTown;
  constexpr int32_t LocalityRadius = 13;
  constexpr int32_t LocalityDiameter = 2 * LocalityRadius + 1;

  enum class LocalityType : uint8_t {
    Farm,
    Camp, // cavalry camp
    Village, // native vilage
  };

  struct LocalityState {
    gf::Vec2I position;
    LocalityType type = LocalityType::Farm;
    uint8_t number = 0;
    gf::Direction direction = gf::Direction::Up;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<LocalityState, Archive>& state)
  {
    return ar | state.position | state.type | state.number | state.direction;
  }

  struct MapState {
    gf::Array2D<MapCell> cells;
    std::array<TownState, TownsCount> towns;
    std::array<LocalityState, LocalityCount> localities;
    gf::Array2D<MapUndergroundCell> underground;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MapState, Archive>& state)
  {
    return ar | state.cells | state.towns | state.localities | state.underground;
  }

}

#endif // FFW_MAP_STATE_H
