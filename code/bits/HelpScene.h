#ifndef FFW_HELP_SCENE_H
#define FFW_HELP_SCENE_H

#include <gf2/core/ActionGroup.h>
#include <gf2/core/ActionSettings.h>
#include <gf2/core/Console.h>
#include <gf2/core/ConsoleScene.h>

namespace ffw {
  class FarFarWest;

  class HelpScene : public gf::ConsoleScene {
  public:
    HelpScene(FarFarWest* game);

    void process_event(const gf::Event& event) override;
    void handle_actions() override;
    // void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    static gf::ActionGroupSettings compute_settings();

    FarFarWest* m_game = nullptr;
    gf::ActionGroup m_action_group;

    gf::Console m_console;
  };

}

#endif // FFW_HELP_SCENE_H
