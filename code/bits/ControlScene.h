#ifndef FFW_CONTROL_SCENE_H
#define FFW_CONTROL_SCENE_H

#include <optional>

#include <gf2/core/ActionGroup.h>
#include <gf2/core/ActionSettings.h>
#include <gf2/core/ConsoleScene.h>
#include <gf2/core/GridMap.h>

#include "Date.h"

namespace ffw {
  class FarFarWest;

  class ControlScene : public gf::ConsoleScene {
  public:
    ControlScene(FarFarWest* game);

    void process_event(const gf::Event& event) override;
    void handle_actions() override;
    // void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    static gf::ActionGroupSettings compute_settings();

    std::vector<gf::Vec2I> compute_path();
    void update_grid();

    FarFarWest* m_game = nullptr;
    gf::ActionGroup m_action_group;
    std::optional<gf::Vec2I> m_mouse;
    gf::GridMap m_grid;
    Date m_last_grid_update = {};
  };

}

#endif // FFW_CONTROL_SCENE_H
