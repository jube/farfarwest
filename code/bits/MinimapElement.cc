#include "MinimapElement.h"

#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  MinimapElement::MinimapElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void MinimapElement::render(gf::Console& console)
  {
    gf::Console minimap = m_game->runtime()->map.minimap;

    const gf::Vec2I hero_position = m_game->state()->hero().position / MinimapFactor;

    gf::ConsoleStyle hero_style;
    hero_style.color.foreground = gf::Black;
    hero_style.color.background = gf::Transparent;
    hero_style.effect = gf::ConsoleEffect::none();

    minimap.put_character(hero_position, u'@', hero_style);

    const int32_t min_extent = std::min(ConsoleSize.x, ConsoleSize.y);
    const gf::RectI hero_box = gf::RectI::from_center_size(hero_position, { min_extent, min_extent });

    minimap.blit_to(console, hero_box, (ConsoleSize - min_extent) / 2);
  }

}
