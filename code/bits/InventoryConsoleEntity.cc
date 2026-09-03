#include "InventoryConsoleEntity.h"

#include <gf2/core/ConsoleOperations.h>

#include "Colors.h"
#include "Settings.h"
#include "FarWest.h"
#include "Styles.h"

namespace fw {

  namespace {

    static_assert(ItemListConsoleSize.y == ItemPreviewConsoleSize.y);

    constexpr gf::Vec2I InventoryConsoleSize = { 2 + ItemListConsoleSize.x + ItemPreviewConsoleSize.x, 2 + ItemPreviewConsoleSize.y };
    constexpr gf::Vec2I InventoryConsolePosition = (GameBoxSize - InventoryConsoleSize) / 2;

    constexpr gf::Vec2I InventoryListPosition = { 1, 1 };
    constexpr gf::Vec2I InventoryPreviewPosition = { 1 + ItemListConsoleSize.x, 1 };

  }

  InventoryConsoleEntity::InventoryConsoleEntity(FarWest* game)
  : m_game(game)
  , m_console(InventoryConsoleSize)
  , m_list_entity(game)
  , m_list_console(ItemListConsoleSize)
  , m_preview_entity(game)
  , m_preview_console(ItemPreviewConsoleSize)
  {
  }

  void InventoryConsoleEntity::set_inventory(const InventoryState* state)
  {
    m_list_entity.set_inventory(state);
  }

  std::optional<int32_t> InventoryConsoleEntity::current_index() const
  {
    return m_list_entity.current_index();
  }

  void InventoryConsoleEntity::update(gf::Time time)
  {
    m_preview_entity.set_item(m_list_entity.current_item());

    m_list_entity.update(time);
    m_preview_entity.update(time);
  }

  void InventoryConsoleEntity::render(gf::Console& console)
  {
    m_preview_entity.set_item(m_list_entity.current_item());

    const gf::ConsoleStyle& style = ui_default_style();

    gf::console_clear(m_console, style);
    gf::console_draw_frame(m_console, gf::RectI::from_size(InventoryConsoleSize), style);

    m_list_entity.render(m_list_console);
    gf::console_blit_to(m_list_console, m_console, InventoryListPosition);

    m_preview_entity.render(m_preview_console);
    gf::console_blit_to(m_preview_console, m_console, InventoryPreviewPosition);

    gf::console_blit_to(m_console, console, InventoryConsolePosition, 1.0f, RpgBlueAlpha);
  }

}
