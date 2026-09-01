#ifndef FW_ITEM_LIST_CONSOLE_ENTITY_H
#define FW_ITEM_LIST_CONSOLE_ENTITY_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleEntity.h>

#include "InventoryState.h"

namespace fw {
  class FarWest;

  constexpr gf::Vec2I ItemListConsoleSize = { 29, 43 };

  class ItemListConsoleEntity : public gf::ConsoleEntity {
  public:
    ItemListConsoleEntity(FarWest* game);

    void set_inventory(const InventoryState* state);
    const ItemData* current_item() const;

    void next_page();
    void prev_page();
    void next_item();
    void prev_item();

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    void normalize_index(int32_t shift);

    FarWest* m_game = nullptr;
    const InventoryState* m_state = nullptr;

    int32_t m_current_page = 0;
    int32_t m_current_offset = 0;
  };

}

#endif // FW_ITEM_LIST_CONSOLE_ENTITY_H
