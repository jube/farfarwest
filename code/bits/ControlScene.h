#ifndef FFW_CONTROL_SCENE_H
#define FFW_CONTROL_SCENE_H

#include <gf2/core/ActionGroup.h>
#include <gf2/core/ActionSettings.h>
#include <gf2/core/ConsoleScene.h>

namespace ffw {
  class FarFarWest;

  class ControlScene : public gf::ConsoleScene {
  public:
    ControlScene(FarFarWest* game);

    void process_event(const gf::Event& event) override;
    void handle_actions() override;
    // void update(gf::Time time) override;

  private:
    static gf::ActionGroupSettings compute_settings();

    FarFarWest* m_game = nullptr;
    gf::ActionGroup m_action_group;
    gf::Vec2I m_orientation = { 0, 0 };
  };

}

#endif // FFW_CONTROL_SCENE_H
