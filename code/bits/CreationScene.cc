#include "CreationScene.h"

#include "FarFarWest.h"

namespace ffw {

  namespace {

    constexpr gf::Vec2I CreationConsoleSize = { 28, 3 };
    constexpr float DotsPerSeconds = 1.5f;

  }

  CreationScene::CreationScene(FarFarWest* game)
  : m_game(game)
  , m_console(CreationConsoleSize)
  {
  }

  void CreationScene::update(gf::Time time)
  {
    m_time += time;

    if (m_game->world_creation_finished()) {
      m_game->start_world();
    }
  }

  void CreationScene::render(gf::Console& console)
  {
    gf::ConsoleStyle style;
    style.color.background = gf::Black;
    style.color.foreground = gf::White;
    style.effect = gf::ConsoleEffect::set();

    m_console.clear(style);
    // m_console.draw_frame(gf::RectI::from_size(CreationConsoleSize), style);

    const std::size_t dots = std::size_t(m_time.as_seconds() * DotsPerSeconds) % 4;
    m_console.print({ 2, 1 }, gf::ConsoleAlignment::Left, style, "Creation of the world" + std::string(dots, '.'));

    const gf::Vec2I padding = console.size() - m_console.size();
    const gf::Vec2I creation_position = { padding.x / 2, padding.y / 2 + 10 };
    m_console.blit_to(console, gf::RectI::from_size(m_console.size()), creation_position);
  }

}
