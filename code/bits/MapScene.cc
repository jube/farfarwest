#include "MapScene.h"

#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  MapScene::MapScene(FarFarWest* game)
  : m_game(game)
  {
  }

  void MapScene::render(gf::Console& console)
  {
    auto state = m_game->state();
    const gf::Vec2I hero_position = state->hero.position;
    const gf::RectI view = gf::RectI::from_center_size(hero_position, GameBoxSize);
    state->map.base.blit_to(console, view, GameBoxPosition);

    gf::ConsoleStyle hero_style;
    hero_style.color.background = gf::Transparent;
    hero_style.color.foreground = gf::Black;
    hero_style.effect = gf::ConsoleEffect::none();

    console.put_character(state->hero.position - view.position(), '@', hero_style);
  }

}
