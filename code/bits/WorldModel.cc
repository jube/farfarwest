#include "WorldModel.h"
#include "MapRuntime.h"
#include "MapState.h"
#include "SchedulerState.h"

#include <cassert>

namespace ffw {

  namespace {

    constexpr gf::Time Cooldown = gf::milliseconds(50);

    constexpr int32_t IdleDistance = 100;
    constexpr uint16_t IdleTime = 100;

    constexpr uint16_t TrainTime = 5;
    constexpr uint16_t WalkTime = 15;
    constexpr uint16_t GrazeTime = 100;


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

  void WorldModel::bind()
  {
    state.bind(data);
    runtime.bind(data, state, m_random);
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

    update_date();

    bool need_cooldown = false;

    while (state.current_date == state.scheduler.queue.top().date) {
      if (state.scheduler.is_hero_turn()) {
        if (update_hero()) {
          gf::Log::debug("[SCHEDULER] {}: Update hero", state.current_date.to_string());
          need_cooldown = true;
        }

        break;
      }

      const Task& current_task = state.scheduler.queue.top();

      if (current_task.type == TaskType::Actor) {
        assert(current_task.index < state.actors.size());
        gf::Log::debug("[SCHEDULER] {}: Update actor {}", state.current_date.to_string(), current_task.index);

        if (update_actor(state.actors[current_task.index])) {
          need_cooldown = true;
        }

        continue;
      }

      if (current_task.type == TaskType::Train) {
        assert(current_task.index < state.network.trains.size());
        gf::Log::debug("[SCHEDULER] {}: Update train {}", state.current_date.to_string(), current_task.index);

        if (update_train(state.network.trains[current_task.index])) {
          need_cooldown = true;
        }

        continue;
      }
    }

    if (need_cooldown) {
      m_phase = Phase::Cooldown;
    }
  }


  bool WorldModel::is_prairie(gf::Vec2I position) const
  {
    if (!state.map.cells.valid(position)) {
      return false;
    }

    return state.map.cells(position).region == MapRegion::Prairie;
  }

  bool WorldModel::is_walkable(gf::Vec2I position) const
  {
    if (!runtime.map.outside_grid.walkable(position)) {
      return false;
    }

    const ReverseMapCell& cell = runtime.map.outside_reverse(position);

    if (cell.actor_index != NoIndex || cell.train_index != NoIndex) {
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

  void WorldModel::update_date()
  {
    state.current_date = state.scheduler.queue.top().date;
  }

  void WorldModel::update_current_task_in_queue(uint16_t seconds)
  {
    Task task = state.scheduler.queue.top();
    state.scheduler.queue.pop();
    task.date.add_seconds(seconds);

    if (task.type != TaskType::Actor || task.index != 0) {
      gf::Log::debug("\tNext turn: {}", task.date.to_string());
    }

    state.scheduler.queue.push(task);
  }

  bool WorldModel::update_hero()
  {
    if (!runtime.hero.moves.empty()) {
      runtime.hero.move(runtime.hero.moves.back() - state.hero().position);
      runtime.hero.moves.pop_back();
    }

    switch (runtime.hero.action.type()) {
      case ActionType::None:
        return false;

      case ActionType::Move:
        {
          MoveAction move = runtime.hero.action.from<ActionType::Move>();
          move.orientation = gf::clamp(move.orientation, -1, +1);
          assert(move.orientation != gf::vec(0, 0));

          const gf::Vec2I new_hero_position = state.hero().position + move.orientation;

          if (is_walkable(new_hero_position)) {
            move_actor(state.hero(), new_hero_position);
            update_current_task_in_queue(WalkTime);
            return true;
          } else {
            runtime.hero.moves.clear();
          }
        }
        break;

    }

    return false;
  }

  bool WorldModel::update_actor(ActorState& actor)
  {
    const int32_t distance_to_hero = gf::chebyshev_distance(actor.position, state.hero().position);

    if (distance_to_hero > IdleDistance) {
      update_current_task_in_queue(distance_to_hero - IdleDistance + IdleTime);
      return false; // do not cooldown in this case
    }

    using namespace gf::literals;

    switch (actor.data->label.id) {
      case "Cow"_id:
        update_cow(actor);
        break;

      default:
        update_current_task_in_queue(10);
        assert(false);
        break;
    }

    return true;
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
    update_current_task_in_queue(GrazeTime);
  }

  bool WorldModel::update_train(TrainState& train)
  {
    const uint32_t new_index = state.network.prev_position(train.index);
    assert(new_index < state.network.railway.size());
    const gf::Vec2I new_position = state.network.railway[new_index];

    const uint32_t last_index = state.network.next_position(train.index, TrainSize - 1);
    assert(last_index < state.network.railway.size());
    const gf::Vec2I last_position = state.network.railway[last_index];

    assert(runtime.map.outside_reverse.valid(last_position));
    ReverseMapCell& old_reverse_cell = runtime.map.outside_reverse(last_position);
    assert(old_reverse_cell.train_index < state.network.trains.size());
    assert(&train == &state.network.trains[old_reverse_cell.train_index]);

    assert(runtime.map.outside_reverse.valid(new_position));
    ReverseMapCell& new_reverse_cell = runtime.map.outside_reverse(new_position);
    assert(runtime.map.outside_reverse(new_position).train_index == NoIndex);

    train.index = new_index;
    std::swap(old_reverse_cell.train_index, new_reverse_cell.train_index);

    if (auto iterator = std::find_if(state.network.stations.begin(), state.network.stations.end(), [new_index](const StationState& station) {
      return station.index == new_index;
    }); iterator != state.network.stations.end()) {
      update_current_task_in_queue(iterator->stop_time);
    } else {
      update_current_task_in_queue(TrainTime);
    }

    const int32_t distance_to_hero = gf::chebyshev_distance(new_position, state.hero().position);

    if (distance_to_hero > IdleDistance) {
      return false; // do not cooldown
    }

    return true;
  }

}
