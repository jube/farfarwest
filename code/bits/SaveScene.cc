#include "SaveScene.h"

#include "Colors.h"
#include "FarFarWest.h"

namespace ffw {

  namespace {

    constexpr gf::Vec2I SaveConsoleSize = { 44, 3 };
    constexpr float DotsPerSeconds = 1.5f;

  }

  SaveScene::SaveScene(FarFarWest* game)
  : m_game(game)
  , m_console(SaveConsoleSize)
  {
  }

  void SaveScene::update(gf::Time time)
  {
    m_time += time;

    if (m_game->save_creation_finished()) {
      m_game->pop_all_scenes();
    }
  }

  void SaveScene::render(gf::Console& console)
  {
    gf::ConsoleStyle style;
    style.color.background = RpgBlue;
    style.color.foreground = gf::White;
    style.effect = gf::ConsoleEffect::set();

    m_console.clear(style);
    m_console.draw_frame(gf::RectI::from_size(SaveConsoleSize), style);

    const std::size_t dots = std::size_t(m_time.as_seconds() * DotsPerSeconds) % 4;
    m_console.print({ 2, 1 }, gf::ConsoleAlignment::Left, style, "Saving the adventure before leaving" + std::string(dots, '.'));

    const gf::Vec2I padding = console.size() - m_console.size();
    const gf::Vec2I save_position = padding / 2;
    m_console.blit_to(console, gf::RectI::from_size(m_console.size()), save_position, 1.0f, RpgBlueAlpha);
  }

}
