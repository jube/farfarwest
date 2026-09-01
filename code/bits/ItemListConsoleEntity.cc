#include "ItemListConsoleEntity.h"

#include <cstdint>

#include <gf2/core/ConsoleOperations.h>
#include <gf2/core/Math.h>

#include "FarWest.h"
#include "ItemState.h"
#include "Styles.h"

namespace fw {

  namespace {

    constexpr int32_t InventoryListLength = 22;
    constexpr int32_t ItemListPageSize = ItemListConsoleSize.y - 3;

  }

  ItemListConsoleEntity::ItemListConsoleEntity(FarWest* game)
  : m_game(game)
  {
  }

  void ItemListConsoleEntity::set_inventory(const InventoryState* state)
  {
    m_state = state;
  }

  const ItemData* ItemListConsoleEntity::current_item() const
  {
    if (m_state == nullptr) {
      return nullptr;
    }

    [[maybe_unused]] const int32_t size = static_cast<int32_t>(m_state->items.size());
    const int32_t index = m_current_page * ItemListPageSize + m_current_offset;

    assert(0 <= index && index < size);

    return m_state->items[index].data.origin;
  }

  void ItemListConsoleEntity::next_page()
  {
    normalize_index(+ItemListPageSize);
  }

  void ItemListConsoleEntity::prev_page()
  {
    normalize_index(-ItemListPageSize);
  }

  void ItemListConsoleEntity::next_item()
  {
    normalize_index(+1);
  }

  void ItemListConsoleEntity::prev_item()
  {
    normalize_index(-1);
  }

  void ItemListConsoleEntity::update([[maybe_unused]] gf::Time time)
  {

  }

  void ItemListConsoleEntity::render(gf::Console& console)
  {
    assert(console.size() == ItemListConsoleSize);

    if (m_state == nullptr) {
      return;
    }

    const gf::ConsoleStyle& style = ui_default_style();
    const gf::ConsoleRichStyle& rich_style = ui_rich_style();

    gf::console_clear(console, style);
    gf::console_draw_frame(console, gf::RectI::from_size(ItemListConsoleSize), style);

    if (!m_state->items.empty()) {
      gf::Vec2I position = { 3, 2 };

      const int32_t size = static_cast<int32_t>(m_state->items.size());
      const int32_t start_index = m_current_page * ItemListPageSize;

      for (int32_t i = 0; i < ItemListPageSize && start_index + i < size; ++i) {
        const InventoryItemState& item = m_state->items[start_index + i];

        if (i == m_current_offset) {
          gf::console_write_picture(console, position - gf::dirx(2), u'\u2192' /* '→' */, style);
        }

        gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "{:.<42}", item.data->label.tag);
        gf::console_print_text(console, position + gf::dirx(InventoryListLength), gf::ConsoleAlignment::Right, rich_style, "{}", item.count);
        ++position.y;
      }

    }
  }

  void ItemListConsoleEntity::normalize_index(int32_t shift)
  {
    if (m_state == nullptr || m_state->items.empty()) {
      m_current_page = 0;
      m_current_offset = 0;
    }

    const int32_t size = static_cast<int32_t>(m_state->items.size());
    [[maybe_unused]] const int32_t page_count = gf::div_ceil(size, ItemListPageSize);

    int32_t index = m_current_page * ItemListPageSize + m_current_offset + shift;

    if (index < 0) {
      index += size;
    } else if (index >= size) {
      index = (index % size);
    }

    assert(0 <= index && index < size);

    m_current_page = index / ItemListPageSize;
    m_current_offset = index % ItemListPageSize;

    assert(0 <= m_current_offset && m_current_offset < ItemListPageSize);
    assert(0 <= m_current_page && m_current_page < page_count);
  }

}
