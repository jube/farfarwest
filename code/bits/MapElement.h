#ifndef FFW_MAP_SCENE_H
#define FFW_MAP_SCENE_H

#include <gf2/core/ConsoleElement.h>

#include "Settings.h"

namespace ffw {
  class FarFarWest;

  class MapElement : public gf::ConsoleElement {
  public:
    MapElement(FarFarWest* game);

    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
    gf::Vec2I m_view_center = WorldSize / 2;
  };

}

#endif // FFW_MAP_SCENE_H
