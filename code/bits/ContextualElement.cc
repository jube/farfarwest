#include "ContextualElement.h"

#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  ContextualElement::ContextualElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void ContextualElement::update([[maybe_unused]] gf::Time time)
  {
  }

  void ContextualElement::render(gf::Console& console)
  {
    auto* state = m_game->state();

    gf::ConsoleStyle contextual_box_style;
    contextual_box_style.color.foreground = gf::Gray;
    console.draw_frame(ContextualBox, contextual_box_style);

  }

}
