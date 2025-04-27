#ifndef FFW_MAP_SCENE_H
#define FFW_MAP_SCENE_H

#include <gf2/core/ConsoleElement.h>

namespace ffw {
  class FarFarWest;

  class MapElement : public gf::ConsoleElement {
  public:
    MapElement(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_MAP_SCENE_H
