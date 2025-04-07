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

    gf::ConsoleStyle character_box_style;
    character_box_style.color.foreground = gf::Yellow;
    console.draw_frame(CharacterBox, character_box_style);

    gf::Vec2I position = CharacterBoxPosition;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=date>{}</>", state->current_date.to_string());

    position += 1;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=cash>$100</>");
    console.print(position + gf::dirx(CharacterBoxSize.x - 3), gf::ConsoleAlignment::Right, m_game->style(), "<style=debt>$10034</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "HP: 42/100");
    ++position.y;


    position = CharacterBox.position_at(gf::Orientation::SouthWest) - gf::diry(5) + gf::dirx(1);

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=key>Weapon:</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "Winchester");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=key>Ammunitions:</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "•••○○○");
  }

}
