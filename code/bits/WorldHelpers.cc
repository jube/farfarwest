#include "WorldHelpers.h"

#include "ItemData.h"
#include "ItemState.h"
#include "WorldModel.h"

namespace fw {

  namespace {

    ItemComponent make_default_component(ItemType type)
    {
      switch (type) {
        case ItemType::None:
          return std::monostate();
        case ItemType::Container:
          return ContainerComponent{};
        case ItemType::MeleeWeapon:
          return MeleeWeaponComponent{};
        case ItemType::DistanceWeapon:
          return DistanceWeaponComponent{};
        case ItemType::Projectile:
          return ProjectileComponent{};
      }

      return {};
    }

    bool weapon_and_projectiles_compatible(EquippedItemState& weapon, InventoryItemState& projectile)
    {
      if (!weapon.data || !projectile.data) {
        return false;
      }

      if (weapon.type() != ItemType::DistanceWeapon) {
        return false;
      }

      assert(projectile.type() == ItemType::Projectile);

      const DistanceWeaponElement& weapon_element = weapon.data->element.from<ItemType::DistanceWeapon>();
      const ProjectileElement& projectile_element = projectile.data->element.from<ItemType::Projectile>();

      return weapon_element.projectile == projectile_element.kind;
    }

    void unequip_item(EquippedItemState& item, InventoryState& inventory)
    {
      auto iterator = std::ranges::find(inventory.items, item.data, &InventoryItemState::data);

      if (iterator != inventory.items.end()) {
        ++iterator->count;
      } else {
        inventory.items.push_back({ .data = item.data, .count = 1 });
      }

      item.data.reset();
      item.component = {};
    }

    void unequip_projectile(InventoryItemState& projectile, InventoryState& inventory)
    {
      assert(projectile.type() == ItemType::Projectile);

      auto iterator = std::ranges::find(inventory.items, projectile.data, &InventoryItemState::data);

      if (iterator != inventory.items.end()) {
        iterator->count += projectile.count;
      } else {
        inventory.items.push_back(projectile);
      }

      projectile.data.reset();
      projectile.count = 0;
    }

    void unequip_projectiles_from_weapon(EquippedItemState& weapon, InventoryItemState& projectile, InventoryState& inventory)
    {
      if (!projectile.data) {
        return;
      }

      assert(weapon.type() == ItemType::DistanceWeapon);
      assert(projectile.type() == ItemType::Projectile);
      assert(weapon_and_projectiles_compatible(weapon, projectile));

      DistanceWeaponComponent& weapon_component = weapon.component.from<ItemType::DistanceWeapon>();
      projectile.count += weapon_component.projectiles;
      weapon_component.projectiles = 0;

      unequip_projectile(projectile, inventory);
    }

    void equip_item(InventoryItemState& inventory_item, EquippedItemState& equipped_item)
    {
      assert(equipped_item.type() == ItemType::None);
      assert(inventory_item.count > 0);
      equipped_item.data = inventory_item.data;
      equipped_item.component = make_default_component(inventory_item.type());
      --inventory_item.count;
    }

    void equip_projectile(InventoryItemState& inventory_item, InventoryItemState& projectile)
    {
      assert(!projectile.data);
      projectile = inventory_item;
      inventory_item.data.reset();
      inventory_item.count = 0;
    }

    void clean_inventory(InventoryState& inventory)
    {
      std::erase_if(inventory.items, [](const InventoryItemState& item) { return !item.data || item.count == 0; });
    }

  }

  void equip_hero(WorldModel& model, int32_t index)
  {
    ActorState& hero = model.state.hero();
    HumanComponent& component = hero.component.from<ActorType::Human>();

    // don't take a reference on the inventory item as the inventory may be modified

    switch (component.inventory.items[index].type()) {
      case ItemType::None:
      case ItemType::Container:
        // these can't be equipped
        return;
      case ItemType::MeleeWeapon:
      case ItemType::DistanceWeapon:
        unequip_projectiles_from_weapon(component.weapon, component.projectile, component.inventory);
        unequip_item(component.weapon, component.inventory);
        equip_item(component.inventory.items[index], component.weapon);
        break;
      case ItemType::Projectile:
        if (weapon_and_projectiles_compatible(component.weapon, component.inventory.items[index])) {
          unequip_projectiles_from_weapon(component.weapon, component.projectile, component.inventory);
          equip_projectile(component.inventory.items[index], component.projectile);
        }
        break;
    }

    clean_inventory(component.inventory);
  }

}
