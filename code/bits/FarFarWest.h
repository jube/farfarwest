#ifndef FFW_FAR_FAR_WEST_H
#define FFW_FAR_FAR_WEST_H

#include <filesystem>
#include <future>

#include <gf2/core/ConsoleSceneManager.h>
#include <gf2/core/ConsoleStyle.h>
#include <gf2/core/Random.h>

#include "ControlScene.h"
#include "GenerationScene.h"
#include "KickoffScene.h"
#include "PrimaryScene.h"
#include "TitleScene.h"
#include "WorldModel.h"

namespace ffw {

  class FarFarWest : public gf::ConsoleSceneManager {
  public:
    FarFarWest(gf::Random* random, const std::filesystem::path& datafile);

    gf::Random* random()
    {
      return m_random;
    }

    WorldModel* model()
    {
      return &m_model;
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

    PrimaryScene primary;
    ControlScene control;


  private:
    gf::Random* m_random = nullptr;
    std::filesystem::path m_datafile;
    WorldModel m_model;
    std::future<void> m_async_generation;
    bool m_async_generation_finished = false;

    gf::ConsoleRichStyle m_rich_style;
  };

}

#endif // FFW_FAR_FAR_WEST_H
