#ifndef FFW_GENERATION_SCENE_H
#define FFW_GENERATION_SCENE_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleScene.h>
#include <gf2/core/Console.h>

namespace ffw {
  class FarFarWest;

  class CreationScene : public gf::ConsoleScene {
  public:
    CreationScene(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
    gf::Time m_time;
    gf::Console m_console;
  };

}

#endif // FFW_GENERATION_SCENE_H
