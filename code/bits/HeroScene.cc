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
    gf::Vec2I position = CharacterBoxPosition;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=date>{}</>", state->current_date.to_string());
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=cash>$100</>     | <style=debt>$10034</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "HP: 42/100");
    ++position.y;


    position = CharacterBox.position_at(gf::Orientation::SouthWest) - gf::diry(4);

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=key>Weapon:</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "Winchester");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=key>Ammunitions:</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "•••○○○");
  }

}
