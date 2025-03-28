#ifndef FFW_GENERATION_SCENE_H
#define FFW_GENERATION_SCENE_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleScene.h>

namespace ffw {
  class FarFarWest;

  class GenerationScene : public gf::ConsoleScene {
  public:
    GenerationScene(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& buffer) override;

  private:
    FarFarWest* m_game = nullptr;
    gf::Time m_time;
  };

}

#endif // FFW_GENERATION_SCENE_H
