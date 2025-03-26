#include "KickoffScene.h"

#include "FarFarWest.h"
#include "Settings.h"
#include "gf2/core/Color.h"

namespace ffw {

  KickoffScene::KickoffScene(FarFarWest* game)
  : m_game(game)
  {
  }

  void KickoffScene::render(gf::Console& console)
  {
    gf::ConsoleStyle style;
    style.color.background = gf::Black;
    style.color.foreground = gf::Red;

    console.draw_frame(MessageBox, style);

    style.color.foreground = gf::Chartreuse;

    console.draw_frame(CharacterBox, style);

    style.color.foreground = gf::Orange;

    console.draw_frame(ContextualBox, style);

    style.color.foreground = gf::White;

    console.print({ 0, 0 }, gf::ConsoleAlignment::Left, style, "Generating world...");

    if (m_game->world_generation_finished()) {
      console.print({ 0, 1 }, gf::ConsoleAlignment::Left, style, "Generation finished!");
    }

    console.put_character({ 30, 30 }, '@');
  }

}
