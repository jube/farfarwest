#include "MapElement.h"

#include <string_view>

#include "ActorState.h"
#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  namespace {

    constexpr int32_t ViewRelaxation = 10;

    constexpr std::u16string_view Train = u"◘█·██·██";
    static_assert(Train.size() == TrainSize);

  }

  MapElement::MapElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void MapElement::update([[maybe_unused]] gf::Time time)
  {
    const auto* state = m_game->state();
    auto* runtime = m_game->runtime();
    const gf::Vec2I hero_position = state->hero().position;
    runtime->view_center = gf::clamp(runtime->view_center, hero_position - ViewRelaxation, hero_position + ViewRelaxation);
  }

  void MapElement::render(gf::Console& console)
  {
    // get current view

    const WorldRuntime* runtime = m_game->runtime();
    const gf::RectI view = runtime->compute_view();

    // display map background

    runtime->map.outside_ground.blit_to(console, view, GameBoxPosition);

    // display actors

    const WorldState* state = m_game->state();

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

    // display trains

    for (const TrainState& train : state->map.network.trains) {
      for (uint32_t i = 0; i < TrainSize; ++i) {
        const uint32_t index = state->map.network.next_position(train.index, i);
        assert(index < state->map.network.railway.size());
        const gf::Vec2I position = state->map.network.railway[index];

        if (!view.contains(position)) {
          continue;
        }

        style.color.foreground = gf::gray(0.2f);
        console.put_character(position - view.position(), Train[i], style);
      }
    }

  }

}
