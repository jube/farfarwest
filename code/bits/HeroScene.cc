#include "HeroScene.h"

#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  HeroScene::HeroScene(FarFarWest* game)
  : m_game(game)
  {
  }

  void HeroScene::update([[maybe_unused]] gf::Time time)
  {
  }

  void HeroScene::render(gf::Console& console)
  {
    auto* state = m_game->state();
    console.print(CharacterBoxPosition, gf::ConsoleAlignment::Left, m_game->style(), "<style=date>{}</>", state->current_date.to_string());
  }

}
