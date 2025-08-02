#include "MinimapElement.h"

#include "FarFarWest.h"
#include "MapRuntime.h"
#include "Settings.h"

namespace ffw {

  MinimapElement::MinimapElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void MinimapElement::zoom_in()
  {
    if (m_zoom_level > 0) {
      --m_zoom_level;
    }
  }

  void MinimapElement::zoom_out()
  {
    if (m_zoom_level < MinimapCount - 1) {
      ++m_zoom_level;
    }
  }

  void MinimapElement::render(gf::Console& console)
  {
    const FloorMap& floor = m_game->runtime()->map.from_floor(m_game->state()->hero().floor);
    const Minimap& minimap = floor.minimaps[m_zoom_level];
    gf::Console minimap_console = minimap.console;

    const gf::Vec2I hero_position = m_game->state()->hero().position / minimap.factor;

    gf::ConsoleStyle hero_style;
    hero_style.color.foreground = gf::Black;
    hero_style.color.background = gf::Transparent;
    hero_style.effect = gf::ConsoleEffect::none();

    minimap_console.put_character(hero_position, u'@', hero_style);

    const int32_t min_extent = std::min(ConsoleSize.x, ConsoleSize.y);
    const gf::RectI hero_box = gf::RectI::from_center_size(hero_position, { min_extent, min_extent });

    minimap_console.blit_to(console, hero_box, (ConsoleSize - min_extent) / 2);
  }

}
