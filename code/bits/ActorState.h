#ifndef FFW_ACTOR_H
#define FFW_ACTOR_H

#include <cstdint>

#include <gf2/core/Color.h>
#include <gf2/core/Fixed.h>
#include <gf2/core/TaggedVariant.h>
#include <gf2/core/Vec2.h>

#include "ActorData.h"
#include "DataReference.h"
#include "Date.h"
#include "Index.h"
#include "InventoryState.h"
#include "MapFloor.h"

namespace ffw {

  enum class Gender : uint8_t {
    Girl,
    Boy,
    NonBinary,
  };

  using Stat = gf::Fixed<int32_t, 16>;

  struct HumanFeature {
    std::string name;
    Gender gender;
    MonthDay birthday;
    int8_t age;
    int8_t health;
    // attributes
    int8_t force;
    int8_t dexterity;
    int8_t constitution;
    int8_t luck; // hidden
    // stats
    Stat intensity;
    Stat precision;
    Stat endurance;
    uint32_t mounting = NoIndex;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<HumanFeature, Archive>& feature)
  {
    return ar | feature.name | feature.gender | feature.birthday | feature.age | feature.health | feature.force | feature.dexterity | feature.constitution | feature.luck | feature.intensity | feature.precision | feature.endurance | feature.mounting;
  }

  struct AnimalFeature {
    uint32_t mounted_by = NoIndex;


  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<AnimalFeature, Archive>& feature)
  {
    return ar | feature.mounted_by;
  }


  using ActorFeature = gf::TaggedVariant<ActorType, HumanFeature, AnimalFeature>;

  struct ActorState {
    DataReference<ActorData> data;
    gf::Vec2I position;
    Floor floor = Floor::Ground;
    ActorFeature feature;
    InventoryState inventory;

    WeaponItemState weapon;
    InventoryItemState ammunition;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<ActorState, Archive>& state)
  {
    return ar | state.data | state.position | state.floor | state.feature | state.inventory | state.weapon | state.ammunition;
  }

}

#endif // FFW_ACTOR_H
