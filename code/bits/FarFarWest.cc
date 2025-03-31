#include "FarFarWest.h"

#include <gf2/core/Log.h>

#include "WorldGeneration.h"
#include "Settings.h"

namespace ffw {

  namespace {

    constexpr gf::ConsoleStyle DefaultStyle = { {  gf::White, gf::Transparent }, gf::ConsoleEffect::none() };

    constexpr gf::ConsoleColorStyle DateStyle = { gf::Yellow, gf::Transparent };
    constexpr gf::ConsoleColorStyle CharacterStyle = { gf::Chartreuse, gf::Transparent };
    constexpr gf::ConsoleColorStyle KeyStyle = { gf::Capri, gf::Transparent };

    gf::ConsoleRichStyle compute_rich_style()
    {
      gf::ConsoleRichStyle style;

      style.set_default_style(DefaultStyle);
      style.set_style("character", CharacterStyle);
      style.set_style("date", DateStyle);
      style.set_style("key", KeyStyle);

      return style;
    }

  }

  FarFarWest::FarFarWest(gf::Random* random)
  : gf::ConsoleSceneManager(ConsoleSize)
  , title(this)
  , kickoff(this)
  , generation(this)
  , message_log(this)
  , hero(this)
  , map(this)
  , m_random(random)
  , m_state()
  , m_rich_style(compute_rich_style())
  {
    push_scene(&title);
    push_scene(&kickoff);
  }

  void FarFarWest::start_world_generation()
  {
    m_async_generation_finished = false;
    m_async_generation = std::async(std::launch::async, [&]() {
      m_state = generate_world(m_random);
    });
  }

  bool FarFarWest::world_generation_finished()
  {
    if (m_async_generation.valid() && m_async_generation.wait_for(std::chrono::seconds::zero()) == std::future_status::ready) {
      m_async_generation.get();
      m_async_generation_finished = true;
    }

    return m_async_generation_finished;
  }

  void FarFarWest::start_world()
  {
    pop_all_scenes();
    push_scene(&message_log);
    push_scene(&hero);
    push_scene(&map);
  }

}
