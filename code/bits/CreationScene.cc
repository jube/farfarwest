#include "CreationScene.h"

#include "FarFarWest.h"
#include "WorldGenerationStep.h"

#include <gf2/core/Color.h>

namespace ffw {

  namespace {

    constexpr gf::Vec2I CreationConsoleSize = { 30, 4 };
    constexpr float DotsPerSeconds = 1.5f;

    std::string_view compute_step(WorldGenerationStep step)
    {
      switch (step) {
        case WorldGenerationStep::Start:
          return "Let's go!";
        case WorldGenerationStep::File:
          return "Loading data file";
        case WorldGenerationStep::Load:
          return "Loading save file";
        case WorldGenerationStep::Date:
          return "Choosing a date";
        case WorldGenerationStep::Terrain:
          return "Generating the terrain";
        case WorldGenerationStep::Biomes:
          return "Determining the biomes";
        case WorldGenerationStep::Moutains:
          return "Creating some moutains";
        case WorldGenerationStep::Towns:
          return "Determining town locations";
        case WorldGenerationStep::Rails:
          return "Consructing railways";
        case WorldGenerationStep::Buildings:
          return "Placing buildings in towns";
        case WorldGenerationStep::Regions:
          return "Listing regions";
        case WorldGenerationStep::Underground:
          return "Digging underground";
        case WorldGenerationStep::Hero:
          return "Raising the hero";
        case WorldGenerationStep::Actors:
          return "Spawning the actors";
        case WorldGenerationStep::Data:
          return "Linking to data";
        case WorldGenerationStep::MapGround:
          return "Setting the map ground";
        case WorldGenerationStep::MapUnderground:
          return "Setting the map underground";
        case WorldGenerationStep::MapRails:
          return "Installing rails";
        case WorldGenerationStep::MapTowns:
          return "Constructing the buildings";
        case WorldGenerationStep::MapMinimap:
          return "Generating the minimaps";
        case WorldGenerationStep::Network:
          return "Tracing the railway network";
        case WorldGenerationStep::End:
          return "The end!";
      }

      return "???";
    }

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
    m_console.draw_frame(gf::RectI::from_size(CreationConsoleSize), style);

    const std::size_t dots = std::size_t(m_time.as_seconds() * DotsPerSeconds) % 4;
    m_console.print({ 3, 1 }, gf::ConsoleAlignment::Left, style, "Creation of the world" + std::string(dots, '.'));

    style.color.foreground = gf::Amber;
    const WorldGenerationStep step = m_game->world_creation_step();
    const std::string_view step_name = compute_step(step);
    m_console.print({ CreationConsoleSize.x / 2, 2 }, gf::ConsoleAlignment::Center, style, step_name);

    const gf::Vec2I padding = console.size() - m_console.size();
    const gf::Vec2I creation_position = { padding.x / 2, padding.y / 2 + 10 };
    m_console.blit_to(console, gf::RectI::from_size(m_console.size()), creation_position);
  }

}
