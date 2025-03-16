#ifndef FFW_FAR_FAR_WEST_SCENE_H
#define FFW_FAR_FAR_WEST_SCENE_H

#include <gf2/core/ActionSettings.h>
#include <gf2/core/ActionGroup.h>
#include <gf2/graphics/ConsoleEntity.h>
#include <gf2/graphics/Scene.h>

#include "FarFarWest.h"
#include "FarFarWestResources.h"

namespace ffw {
  class FarFarWestSystem;

  class FarFarWestScene : public gf::Scene {
  public:
    FarFarWestScene(FarFarWestSystem* game, const FarFarWestResources& resources);

  private:
    static gf::ActionGroupSettings compute_settings();

    void do_process_event(const gf::Event& event) override;
    void do_handle_actions() override;
    void do_update(gf::Time time) override;

    FarFarWestSystem* m_game;
    gf::ActionGroup m_action_group;
    FarFarWest m_console_scene_manager;
    gf::ConsoleEntity m_console_entity;
  };

}

#endif // FFW_FAR_FAR_WEST_SCENE_H
