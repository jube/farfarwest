#ifndef FFW_FAR_FAR_WEST_H
#define FFW_FAR_FAR_WEST_H

#include <cstdint>

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

  enum class AdventureChoice : uint8_t {
    New,
    Saved,
  };

  class FarFarWest : public gf::ConsoleSceneManager {
  public:
    FarFarWest(gf::Random* random, const std::filesystem::path& datafile, const std::filesystem::path& savefile);

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

    void start_world_generation(AdventureChoice choice);
    bool world_generation_finished();

    void start_world();

    bool has_save() const;
    void save();

    TitleScene title;
    KickoffScene kickoff;
    GenerationScene generation;

    PrimaryScene primary;
    ControlScene control;


  private:
    gf::Random* m_random = nullptr;
    std::filesystem::path m_datafile;
    std::filesystem::path m_savefile;
    WorldModel m_model;
    std::future<void> m_async_generation;
    bool m_async_generation_finished = false;

    gf::ConsoleRichStyle m_rich_style;
  };

}

#endif // FFW_FAR_FAR_WEST_H
