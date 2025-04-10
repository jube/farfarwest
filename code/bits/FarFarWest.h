#ifndef FFW_FAR_FAR_WEST_H
#define FFW_FAR_FAR_WEST_H

#include <cstdint>

#include <filesystem>
#include <future>

#include <gf2/core/ConsoleSceneManager.h>
#include <gf2/core/ConsoleStyle.h>
#include <gf2/core/Random.h>

#include "ControlScene.h"
#include "CreationScene.h"
#include "KickoffScene.h"
#include "PrimaryScene.h"
#include "QuitScene.h"
#include "SaveScene.h"
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

    void create_world(AdventureChoice choice);
    bool world_creation_finished();

    void start_world();

    bool has_save() const;
    void create_save();
    bool save_creation_finished();

    TitleScene title;
    KickoffScene kickoff;
    CreationScene creation;

    PrimaryScene primary;
    ControlScene control;
    QuitScene quit;
    SaveScene save;

  private:
    gf::Random* m_random = nullptr;
    std::filesystem::path m_datafile;

    std::filesystem::path m_savefile;
    std::future<void> m_async_save;
    bool m_async_save_finished = false;

    WorldModel m_model;
    std::future<void> m_async_world;
    bool m_async_world_finished = false;

    gf::ConsoleRichStyle m_rich_style;
  };

}

#endif // FFW_FAR_FAR_WEST_H
