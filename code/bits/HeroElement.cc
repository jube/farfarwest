#include "HeroElement.h"

#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  HeroElement::HeroElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void HeroElement::update([[maybe_unused]] gf::Time time)
  {
  }

  void HeroElement::render(gf::Console& console)
  {
    auto* state = m_game->state();

    gf::ConsoleStyle character_box_style;
    character_box_style.color.foreground = gf::Gray;
    console.draw_frame(CharacterBox, character_box_style);

    gf::Vec2I position = CharacterBoxPosition;
    --position.y;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=date>{}</>", state->current_date.to_string());

    position = CharacterBoxPosition + 1;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=cash>$100</>");
    console.print(position + gf::dirx(CharacterBoxSize.x - 3), gf::ConsoleAlignment::Right, m_game->style(), "<style=debt>$10034</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=stat>HP</>: 42%");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=stat>For</>: 18");
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=stat>Dex</>: 16");
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=stat>Con</>: 12");
    ++position.y;

    position = CharacterBox.position_at(gf::Orientation::SouthWest) - gf::diry(5) + gf::dirx(1);

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=item>Weapon:</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "Winchester");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=item>Ammunitions:</>");
    ++position.y;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "•••○○○");
  }

}
