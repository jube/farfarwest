#ifndef FFW_MINIMAP_ELEMENT_H
#define FFW_MINIMAP_ELEMENT_H

#include <gf2/core/ConsoleElement.h>

namespace ffw {
  class FarFarWest;

  class MinimapElement : public gf::ConsoleElement {
  public:
    MinimapElement(FarFarWest* game);

    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_MINIMAP_ELEMENT_H
