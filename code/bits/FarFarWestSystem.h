#ifndef FFW_FAR_FAR_WEST_SYSTEM_H
#define FFW_FAR_FAR_WEST_SYSTEM_H

#include <filesystem>
#include <memory>

#include <gf2/framework/SceneSystem.h>

#include "FarFarWestResources.h"
#include "FarFarWestScene.h"

namespace ffw {

  class FarFarWestSystem : public gf::SceneSystem {
  public:
    FarFarWestSystem(const std::filesystem::path& asset_directory);

  private:
    FarFarWestResources m_resources;
    std::unique_ptr<FarFarWestScene> m_scene;
  };

}

#endif // FFW_FAR_FAR_WEST_SYSTEM_H
