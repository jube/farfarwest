#ifndef FFW_CONTEXTUAL_ELEMENT_H
#define FFW_CONTEXTUAL_ELEMENT_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleElement.h>

namespace ffw {
  class FarFarWest;

  class ContextualElement : public gf::ConsoleElement {
  public:
    ContextualElement(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_CONTEXTUAL_ELEMENT_H
