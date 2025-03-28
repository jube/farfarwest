#include "FarFarWest.h"

#include <gf2/core/Log.h>

#include "WorldGeneration.h"
#include "Settings.h"

namespace ffw {

  FarFarWest::FarFarWest(gf::Random* random)
  : gf::ConsoleSceneManager(ConsoleSize)
  , title(this)
  , kickoff(this)
  , generation(this)
  , m_random(random)
  , m_state()
  {
    // gf::ConsoleStyle style;
    // style.color.background = gf::Capri;
    // console().clear(style);

    push_scene(&title);
    push_scene(&kickoff);
    // start_world_generation();
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

}
