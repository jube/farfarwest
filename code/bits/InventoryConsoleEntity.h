#ifndef FW_INVENTORY_CONSOLE_ENTITY_H
#define FW_INVENTORY_CONSOLE_ENTITY_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleEntity.h>

#include "ItemListConsoleEntity.h"
#include "ItemPreviewConsoleEntity.h"
#include "InventoryState.h"

namespace fw {
  class FarWest;

  class InventoryConsoleEntity : public gf::ConsoleEntity {
  public:
    InventoryConsoleEntity(FarWest* game);

    void set_inventory(const InventoryState* state);
    std::optional<int32_t> current_index() const;

    void next_page() { m_list_entity.next_page(); }
    void prev_page() { m_list_entity.prev_page(); }
    void next_item() { m_list_entity.next_item(); }
    void prev_item() { m_list_entity.prev_item(); }

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarWest* m_game = nullptr;
    gf::Console m_console;
    ItemListConsoleEntity m_list_entity;
    gf::Console m_list_console;
    ItemPreviewConsoleEntity m_preview_entity;
    gf::Console m_preview_console;
  };

}

#endif // FW_INVENTORY_CONSOLE_ENTITY_H
