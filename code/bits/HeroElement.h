#ifndef FFW_HERO_SCENE_H
#define FFW_HERO_SCENE_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleElement.h>

namespace ffw {
  class FarFarWest;

  class HeroElement : public gf::ConsoleElement {
  public:
    HeroElement(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_HERO_SCENE_H
