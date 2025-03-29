#ifndef FFW_HERO_SCENE_H
#define FFW_HERO_SCENE_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleScene.h>

namespace ffw {
  class FarFarWest;

  class HeroScene : public gf::ConsoleScene {
  public:
    HeroScene(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_HERO_SCENE_H
