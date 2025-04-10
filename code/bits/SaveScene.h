#ifndef FFW_SAVE_SCENE_H
#define FFW_SAVE_SCENE_H

#include <gf2/core/ConsoleScene.h>
#include <gf2/core/Console.h>

namespace ffw {
  class FarFarWest;

  class SaveScene : public gf::ConsoleScene {
  public:
    SaveScene(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
    gf::Time m_time;
    gf::Console m_console;
  };

}

#endif // FFW_SAVE_SCENE_H
