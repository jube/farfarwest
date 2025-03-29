#include "GenerationScene.h"

#include "FarFarWest.h"

namespace ffw {

  namespace {

    constexpr float DotsPerSeconds = 1.5f;

  }

  GenerationScene::GenerationScene(FarFarWest* game)
  : m_game(game)
  {
  }

  void GenerationScene::update(gf::Time time)
  {
    m_time += time;
  }

  void GenerationScene::render(gf::Console& console)
  {
    gf::ConsoleStyle style;
    style.color.background = gf::Black;
    style.color.foreground = gf::White;


    if (!m_game->world_generation_finished()) {
      std::size_t dots = std::size_t(m_time.as_seconds() * DotsPerSeconds) % 4;
      console.print({ 35, 35 }, gf::ConsoleAlignment::Left, style, "Generating world" + std::string(dots, '.'));
    } else {
      m_game->pop_all_scenes();
      m_game->push_scene(&m_game->map);
      m_game->push_scene(&m_game->control);
    }
  }

}
