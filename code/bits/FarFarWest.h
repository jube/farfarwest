#ifndef FFW_FAR_FAR_WEST_H
#define FFW_FAR_FAR_WEST_H

#include <future>

#include <gf2/core/ConsoleSceneManager.h>
#include <gf2/core/ConsoleStyle.h>
#include <gf2/core/Random.h>

#include "GenerationScene.h"
#include "HeroScene.h"
#include "KickoffScene.h"
#include "MapScene.h"
#include "MessageLogScene.h"
#include "TitleScene.h"
#include "WorldModel.h"

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
      return &m_model.state;
    }

    WorldRuntime* runtime()
    {
      return &m_model.runtime;
    }

    const gf::ConsoleRichStyle& style() const
    {
      return m_rich_style;
    }

    void start_world_generation();
    bool world_generation_finished();

    void start_world();

    TitleScene title;
    KickoffScene kickoff;
    GenerationScene generation;

    MessageLogScene message_log;
    HeroScene hero;
    MapScene map;


  private:
    gf::Random* m_random = nullptr;
    WorldModel m_model;
    std::future<void> m_async_generation;
    bool m_async_generation_finished = false;

    gf::ConsoleRichStyle m_rich_style;
  };

}

#endif // FFW_FAR_FAR_WEST_H
