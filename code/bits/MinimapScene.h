#ifndef FFW_MINIMAP_SCENE_H
#define FFW_MINIMAP_SCENE_H

#include <gf2/core/ActionGroup.h>
#include <gf2/core/ActionSettings.h>
#include <gf2/core/ConsoleScene.h>

#include "MinimapElement.h"

namespace ffw {
  class FarFarWest;

  class MinimapScene : public gf::ConsoleScene {
  public:
    MinimapScene(FarFarWest* game);

    void process_event(const gf::Event& event) override;
    void handle_actions() override;

  private:
    static gf::ActionGroupSettings compute_settings();

    FarFarWest* m_game = nullptr;
    gf::ActionGroup m_action_group;

    MinimapElement m_minimap;
  };

}

#endif // FFW_MINIMAP_SCENE_H
