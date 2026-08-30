#ifndef FW_ACTOR_H
#define FW_ACTOR_H

#include <cstdint>

#include <gf2/core/Color.h>
#include <gf2/core/Fixed.h>
#include <gf2/core/TaggedVariant.h>
#include <gf2/core/Vec2.h>

#include "ActorData.h"
#include "BodyState.h"
#include "Combat.h"
#include "DataReference.h"
#include "Date.h"
#include "Index.h"
#include "InventoryState.h"
#include "Location.h"

namespace fw {

  enum class Gender : uint8_t {
    Girl,
    Boy,
    NonBinary,
  };

  struct HumanComponent {
    std::string name;
    Location location;
    Gender gender;
    MonthDay birthday;
    int8_t age;
    BodyState body;
    uint32_t mounting = NoIndex;
    InventoryState inventory;

    EquippedItemState weapon;
    InventoryItemState projectile;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<HumanComponent, Archive>& component)
  {
    return ar | component.name | component.location | component.gender | component.birthday | component.age | component.body | component.mounting | component.inventory | component.weapon | component.projectile;
  }

  struct AnimalComponent {
    Location location;
    BodyState body;
    uint32_t mounted_by = NoIndex;
    InventoryState inventory;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<AnimalComponent, Archive>& component)
  {
    return ar | component.location | component.body | component.mounted_by | component.inventory;
  }

  enum class GroupType : uint8_t {
    Herd, // for herbivores
    Pack, // for carnivores
  };

  struct GroupComponent {
    GroupType type;
    std::vector<uint32_t> members;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<GroupComponent, Archive>& component)
  {
    return ar | component.type | component.members;
  }

  struct TrainComponent {
    uint32_t railway_index;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<TrainComponent, Archive>& component)
  {
    return ar | component.railway_index;
  }

  using ActorComponent = gf::TaggedVariant<ActorType, HumanComponent, AnimalComponent, GroupComponent, TrainComponent>;

  struct ActorState {
    DataReference<ActorData> data;
    ActorComponent component;

    ActorType type() const { return component.type(); }
    Location location() const;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<ActorState, Archive>& state)
  {
    return ar | state.data | state.component;
  }

  AttackDigest compute_attack_digest(const ActorState& actor);
  DefenseDigest compute_defense_digest(const ActorState& actor);

}

#endif // FW_ACTOR_H
