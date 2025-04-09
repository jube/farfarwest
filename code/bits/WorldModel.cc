#include "WorldModel.h"

#include <cassert>

namespace ffw {

  namespace {

    constexpr gf::Time Cooldown = gf::milliseconds(100);


    gf::Orientation random_orientation(gf::Random* random) {
      constexpr gf::Orientation Orientations[] = {
        gf::Orientation::North,
        gf::Orientation::NorthEast,
        gf::Orientation::East,
        gf::Orientation::SouthEast,
        gf::Orientation::South,
        gf::Orientation::SouthWest,
        gf::Orientation::West,
        gf::Orientation::NorthWest,
      };

      const std::size_t index = random->compute_uniform_integer(std::size(Orientations));
      assert(index < std::size(Orientations));
      return Orientations[index];
    }

  }

  WorldModel::WorldModel(gf::Random* random)
  : m_random(random)
  {
  }

  void WorldModel::update(gf::Time time)
  {
    if (m_phase == Phase::Cooldown) {
      m_cooldown += time;

      if (m_cooldown > Cooldown) {
        m_cooldown = gf::Time();
        m_phase = Phase::Running;
      }

      return;
    }

    state.current_date = state.scheduler.queue.top().date;

    while (state.current_date == state.scheduler.queue.top().date) {
      if (state.scheduler.is_hero_turn()) {
        if (!update_hero()) {
          break;
        }

        continue;
      }

      const Task& current_task = state.scheduler.queue.top();
      ActorState& current_actor = state.actors[current_task.index];

      using namespace gf::literals;

      switch (current_actor.data->label.id) {
        case "Cow"_id:
          update_cow(current_actor);
          break;

        default:
          update_current_actor_in_queue(10);
          assert(false);
          break;
      }

      break;
    }

    m_phase = Phase::Cooldown;
  }

  bool WorldModel::is_prairie(gf::Vec2I position) const
  {
    if (!state.map.cells.valid(position)) {
      return false;
    }

    return state.map.cells(position).trait.region == MapRegion::Prairie;
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

  void WorldModel::update_current_actor_in_queue(uint16_t seconds)
  {
    Task task = state.scheduler.queue.top();
    state.scheduler.queue.pop();
    task.date.add_seconds(seconds);
    state.scheduler.queue.push(task);
  }

  bool WorldModel::update_hero()
  {
    if (runtime.hero.orientation != gf::vec(0, 0)) {
      runtime.hero.orientation = gf::clamp(runtime.hero.orientation, -1, +1);

      const gf::Vec2I new_hero_position = state.hero().position + runtime.hero.orientation;
      runtime.hero.orientation = { 0, 0 };

      if (is_walkable(new_hero_position)) {
        move_actor(state.hero(), new_hero_position);
        update_current_actor_in_queue(15);
        return true;
      }
    }

    return false;
  }

  void WorldModel::update_cow(ActorState& cow)
  {
    gf::Orientation orientation = random_orientation(m_random);
    gf::Vec2I new_position = cow.position + gf::displacement(orientation);

    while (!is_walkable(new_position) || !is_prairie(new_position)) {
      orientation = random_orientation(m_random);
      new_position = cow.position + gf::displacement(orientation);
    }

    move_actor(cow, new_position);
    update_current_actor_in_queue(100);
  }

}
