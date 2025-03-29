#ifndef FFW_KICKOFF_SCENE_H
#define FFW_KICKOFF_SCENE_H

#include <gf2/core/ActionSettings.h>
#include <gf2/core/ActionGroup.h>
#include <gf2/core/ConsoleScene.h>

namespace ffw {
  class FarFarWest;

  class KickoffScene : public gf::ConsoleScene {
  public:
    KickoffScene(FarFarWest* game);

    void process_event(const gf::Event& event) override;
    void handle_actions() override;
    void render(gf::Console& console) override;

  private:
    static gf::ActionGroupSettings compute_settings();

    FarFarWest* m_game = nullptr;
    gf::ActionGroup m_action_group;
    bool m_has_saved_game = false;
    int m_choice = 0;
  };

}

#endif // FFW_KICKOFF_SCENE_H
