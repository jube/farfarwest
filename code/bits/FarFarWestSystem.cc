#include "FarFarWestSystem.h"

namespace ffw {

  FarFarWestSystem::FarFarWestSystem(const std::filesystem::path& asset_directory)
  : gf::SceneSystem("Far Far West", { 1600, 900 }, asset_directory)
  {
    auto bundle = m_resources.bundle(this);
    bundle.load_from(resource_manager());

    m_scene = std::make_unique<FarFarWestScene>(this, m_resources);

    push_scene(m_scene.get());
  }

}
