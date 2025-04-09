#include "MapElement.h"

#include "ActorState.h"
#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  namespace {

    constexpr int32_t ViewRelaxation = 10;

  }

  MapElement::MapElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void MapElement::render(gf::Console& console)
  {
    const auto* state = m_game->state();
    const auto* runtime = m_game->runtime();

    const gf::Vec2I hero_position = state->hero().position;

    m_view_center = gf::clamp(m_view_center, hero_position - ViewRelaxation, hero_position + ViewRelaxation);

    const gf::RectI view = gf::RectI::from_center_size(m_view_center, GameBoxSize);
    runtime->map.outside_ground.blit_to(console, view, GameBoxPosition);

    gf::ConsoleStyle style;
    style.color.background = gf::Transparent;
    style.effect = gf::ConsoleEffect::none();

    for (const ActorState& actor : state->actors) {
      if (!view.contains(actor.position)) {
        continue;
      }

      style.color.foreground = actor.data->color;
      console.put_character(actor.position - view.position(), actor.data->picture, style);
    }

  }

}
