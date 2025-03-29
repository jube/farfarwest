#ifndef FFW_MAP_SCENE_H
#define FFW_MAP_SCENE_H

#include <gf2/core/ConsoleScene.h>

namespace ffw {
  class FarFarWest;

  class MapScene : public gf::ConsoleScene {
  public:
    MapScene(FarFarWest* game);

    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_MAP_SCENE_H
