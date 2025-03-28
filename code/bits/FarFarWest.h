#ifndef FFW_FAR_FAR_WEST_H
#define FFW_FAR_FAR_WEST_H

#include <future>

#include <gf2/core/ConsoleSceneManager.h>
#include <gf2/core/Random.h>

#include "GenerationScene.h"
#include "KickoffScene.h"
#include "TitleScene.h"
#include "WorldState.h"

namespace ffw {

  class FarFarWest : public gf::ConsoleSceneManager {
  public:
    FarFarWest(gf::Random* random);

    gf::Random* random()
    {
      return m_random;
    }

    WorldState* state()
    {
      return &m_state;
    }

    void start_world_generation();
    bool world_generation_finished();

    TitleScene title;
    KickoffScene kickoff;
    GenerationScene generation;

  private:
    gf::Random* m_random = nullptr;
    WorldState m_state;
    std::future<void> m_async_generation;
    bool m_async_generation_finished = false;
  };

}

#endif // FFW_FAR_FAR_WEST_H
