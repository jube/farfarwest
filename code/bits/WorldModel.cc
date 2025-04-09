#include "WorldModel.h"

#include <cassert>

namespace ffw {

  void WorldModel::update([[maybe_unused]] gf::Time time)
  {
    if (runtime.hero.orientation != gf::vec(0, 0)) {
      runtime.hero.orientation = gf::clamp(runtime.hero.orientation, -1, +1);

      const gf::Vec2I new_hero_position = state.hero().position + runtime.hero.orientation;

      if (is_walkable(new_hero_position)) {
        move_actor(state.hero(), new_hero_position);
        state.current_date.add_seconds(15);
      }

      runtime.hero.orientation = { 0, 0 };
    }

  }

  bool WorldModel::is_walkable(gf::Vec2I position) const
  {
    if (!runtime.map.outside_grid.walkable(position)) {
      return false;
    }

    if (runtime.map.outside_reverse(position).actor_index != NoIndex) {
      return false;
    }

    return true;
  }

  void WorldModel::move_actor(ActorState& actor, gf::Vec2I position)
  {
    assert(gf::chebyshev_distance(actor.position, position) < 2);

    assert(runtime.map.outside_reverse.valid(actor.position));
    ReverseMapCell& old_reverse_cell = runtime.map.outside_reverse(actor.position);
    assert(old_reverse_cell.actor_index < state.actors.size());
    assert(&actor == &state.actors[old_reverse_cell.actor_index]);

    assert(runtime.map.outside_reverse.valid(position));
    ReverseMapCell& new_reverse_cell = runtime.map.outside_reverse(position);
    assert(runtime.map.outside_reverse(position).actor_index == NoIndex);

    actor.position = position;
    std::swap(old_reverse_cell.actor_index, new_reverse_cell.actor_index);
  }

}
