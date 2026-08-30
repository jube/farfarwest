#ifndef FW_ITEM_DATA_H
#define FW_ITEM_DATA_H

#include <cstdint>

#include <nlohmann/json.hpp>

#include <gf2/core/Color.h>
#include <gf2/core/Console.h>
#include <gf2/core/TaggedVariant.h>

#include "Combat.h"
#include "DisplayData.h"
#include "DataLabel.h"
#include "DateTypes.h"

namespace fw {
  struct ItemData;

  enum class ItemType {
    None,
    Container,
    MeleeWeapon,
    DistanceWeapon,
    Projectile,
  };

  std::string_view to_string(ItemType type);

  enum class ProjectileKind : uint8_t {
    None,
    Dot36Ammunition,
    Dot44Ammunition,
  };

  std::string_view to_string(ProjectileKind kind);

  struct ContainerElement {
    int16_t capacity;
  };

  struct MeleeWeaponElement {
    Attack attack;
    Second use_time;
    int8_t modifier;
  };

  struct DistanceWeaponElement {
    int32_t range;
    Second shoot_time;
    Second reload_time;
    int16_t capacity;
    int8_t modifier;
    ProjectileKind projectile;
  };

  struct ProjectileElement {
    DisplayData empty;
    Attack attack;
    ProjectileKind kind;
    int8_t modifier;
  };

  using ItemElement = gf::TaggedVariant<ItemType, ContainerElement, MeleeWeaponElement, DistanceWeaponElement, ProjectileElement>;

  struct ItemData {
    DataLabel label;
    DisplayData display;
    gf::Console image;
    ItemElement element;

    ItemType type() const { return element.type(); }
  };

  void from_json(const nlohmann::json& j, ItemData& data);

}

#endif // FW_ITEM_DATA_H
