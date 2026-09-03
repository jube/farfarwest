#ifndef FW_ITEM_STATE_H
#define FW_ITEM_STATE_H

#include <vector>

#include <gf2/core/Vec2.h>

#include "DataReference.h"
#include "ItemData.h"
#include "Location.h"

namespace fw {

  struct ContainerComponent {
    std::vector<DataReference<ItemData>> content;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<ContainerComponent, Archive>& component)
  {
    return ar | component.content;
  }

  struct MeleeWeaponComponent {
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, [[maybe_unused]] gf::MaybeConst<MeleeWeaponComponent, Archive>& component)
  {
    return ar;
  }

  struct DistanceWeaponComponent {
    int16_t projectiles = 0;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<DistanceWeaponComponent, Archive>& component)
  {
    return ar | component.projectiles;
  }

  struct ProjectileComponent {
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, [[maybe_unused]] gf::MaybeConst<ProjectileComponent, Archive>& component)
  {
    return ar;
  }

  using ItemComponent = gf::TaggedVariant<ItemType, ContainerComponent, MeleeWeaponComponent, DistanceWeaponComponent, ProjectileComponent>;

  struct ItemState {
    DataReference<ItemData> data;
    ItemComponent component;
    Location location;

    ItemType type() const { return component.type(); }
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<ItemState, Archive>& state)
  {
    return ar | state.data | state.component | state.location;
  }

  struct InventoryItemState {
    DataReference<ItemData> data;
    int16_t count = 0;

    ItemType type() const { return data->type(); }
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<InventoryItemState, Archive>& state)
  {
    return ar | state.data | state.count;
  }

  struct EquippedItemState {
    DataReference<ItemData> data;
    ItemComponent component;

    ItemType type() const { return component.type(); }
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<EquippedItemState, Archive>& state)
  {
    return ar | state.data | state.component;
  }

}

#endif // FW_ITEM_STATE_H
