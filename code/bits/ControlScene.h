#ifndef FFW_CONTROL_SCENE_H
#define FFW_CONTROL_SCENE_H

#include <optional>

#include <gf2/core/ActionGroup.h>
#include <gf2/core/ActionSettings.h>
#include <gf2/core/ConsoleScene.h>

#include "Date.h"
#include "MapRuntime.h"

namespace ffw {
  class FarFarWest;

  class ControlScene : public gf::ConsoleScene {
  public:
    ControlScene(FarFarWest* game);

    void process_event(const gf::Event& event) override;
    void handle_actions() override;
    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    static gf::ActionGroupSettings compute_settings();

    void update_grid();

    FarFarWest* m_game = nullptr;
    gf::ActionGroup m_action_group;
    std::optional<gf::Vec2I> m_mouse;

    Date m_last_grid_update = {};
    gf::Array2D<RuntimeMapCell> m_grid;
    std::vector<gf::Vec2I> m_computed_path;
  };

}

#endif // FFW_CONTROL_SCENE_H
