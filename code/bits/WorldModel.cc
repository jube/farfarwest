#include "WorldModel.h"

#include <cassert>
#include <cstdint>

#include <gf2/core/FieldOfVision.h>

#include "ActorData.h"
#include "ActorState.h"
#include "Index.h"
#include "MapCell.h"
#include "MapRuntime.h"
#include "MapState.h"
#include "SchedulerState.h"
#include "Times.h"
#include "WorldGenerationStep.h"

namespace ffw {

  namespace {

    constexpr gf::Time Cooldown = gf::milliseconds(20);

    constexpr int32_t IdleDistance = 100;

    constexpr int MaxMoveTries = 10;

    gf::Orientation random_orientation(gf::Random* random) {
      constexpr gf::Orientation Orientations[] = {
        gf::Orientation::Center,
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

    constexpr Floor compute_floor_down(Floor floor)
    {
      switch (floor) {
        case Floor::Underground:
          return Floor::Underground;
        case Floor::Ground:
          return Floor::Underground;
        case Floor::Upstairs:
          return Floor::Ground;
      }

      assert(false);
      return Floor::Ground;
    }

    constexpr Floor compute_floor_up(Floor floor)
    {
      switch (floor) {
        case Floor::Underground:
          return Floor::Ground;
        case Floor::Ground:
          return Floor::Upstairs;
        case Floor::Upstairs:
          return Floor::Upstairs;
      }

      assert(false);
      return Floor::Ground;
    }

  }

  WorldModel::WorldModel(gf::Random* random)
  : m_random(random)
  {
  }

  void WorldModel::bind(std::atomic<WorldGenerationStep>& step)
  {
    step.store(WorldGenerationStep::Data);
    state.bind(data);
    runtime.bind(data, state, m_random, step);
  }

  void WorldModel::update(gf::Time time)
  {
    if (m_phase == Phase::Cooldown) {
      m_cooldown += time;

      if (m_cooldown > Cooldown) {
        m_cooldown -= Cooldown;
        m_phase = Phase::Running;
      }

      return;
    }

    update_date();

    bool need_cooldown = false;

    while (state.current_date == state.scheduler.queue.top().date) {
      if (state.scheduler.is_hero_turn()) {
        if (update_hero()) {
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

        if (update_train(state.network.trains[current_task.index], current_task.index)) {
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
    if (!state.map.ground.valid(position)) {
      return false;
    }

    return state.map.ground(position).region == MapCellBiome::Prairie;
  }

  bool WorldModel::is_walkable(Floor floor, gf::Vec2I position) const
  {
    const FloorMap& floor_map = runtime.map.from_floor(floor);

    if (!floor_map.background(position).walkable()) {
      return false;
    }

    assert(floor_map.reverse.valid(position));
    const ReverseMapCell& cell = floor_map.reverse(position);

    if (!cell.empty()) {
      return false;
    }

    return true;
  }

  void WorldModel::move_actor(ActorState& actor, gf::Vec2I position)
  {
    assert(gf::chebyshev_distance(actor.position, position) < 2);

    if (actor.position == position) {
      return;
    }

    FloorMap& floor_map = runtime.map.from_floor(actor.floor);

    assert(floor_map.reverse.valid(actor.position));
    ReverseMapCell& old_reverse_cell = floor_map.reverse(actor.position);
    assert(old_reverse_cell.actor_index < state.actors.size());
    assert(&actor == &state.actors[old_reverse_cell.actor_index]);

    assert(floor_map.reverse.valid(position));
    ReverseMapCell& new_reverse_cell = floor_map.reverse(position);
    assert(floor_map.reverse(position).actor_index == NoIndex);

    actor.position = position;
    std::swap(old_reverse_cell.actor_index, new_reverse_cell.actor_index);
  }

  bool WorldModel::move_human(ActorState& actor, gf::Vec2I position)
  {
    assert(actor.feature.type() == ActorType::Human);

    if (!is_walkable(actor.floor, position)) {
      return false;
    }

    const int32_t move_length = gf::manhattan_length(actor.position - position);

    const uint32_t mount_index = actor.feature.from<ActorType::Human>().mounting;

    if (mount_index == NoIndex) {
      // the human is not mouting an animal

      move_actor(actor, position);

      if (move_length == 2) {
        update_current_task_in_queue(DiagonalWalkTime);
      } else if (move_length == 1) {
        update_current_task_in_queue(StraightWalkTime);
      }
    } else {
      // the humain is mouting an animal

      ActorState& mount = state.actors[mount_index];
      move_actor(mount, position);
      actor.position = position;

      if (move_length == 2) {
        update_current_task_in_queue(DiagonalWalkTime); // TODO: change the time according to mount
      } else if (move_length == 1) {
        update_current_task_in_queue(StraightWalkTime);
      }
    }

    return true;
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

    gf::Log::debug("\tNext turn: {}", task.date.to_string());

    state.scheduler.queue.push(task);
  }

  bool WorldModel::update_hero()
  {
    if (!runtime.hero.moves.empty()) {
      runtime.hero.move(runtime.hero.moves.back() - state.hero().position);
      runtime.hero.moves.pop_back();
    }

    if (runtime.hero.action.type() == ActionType::None) {
      return false;
    }

    gf::Log::debug("[SCHEDULER] {}: Update hero", state.current_date.to_string());
    bool need_cooldown = false;

    switch (runtime.hero.action.type()) {
      case ActionType::None:
        assert(false);
        return false;

      case ActionType::Idle:
        update_current_task_in_queue(HeroIdleTime);
        need_cooldown = false;
        break;

      case ActionType::Move:
        {
          ActorState& hero = state.hero();

          MoveAction move = runtime.hero.action.from<ActionType::Move>();
          move.orientation = gf::clamp(move.orientation, -1, +1);
          const gf::Vec2I new_hero_position = state.hero().position + move.orientation;

          if (move_human(hero, new_hero_position)) {
            need_cooldown = true;

            BackgroundMap& map = state.map.from_floor(hero.floor);
            clear_visible(map);

            gf::compute_symmetric_shadowcasting(map, map, new_hero_position, HeroVisionRange, [](MapCell& cell) {
              cell.properties.set(MapCellProperty::Visible);
              cell.properties.set(MapCellProperty::Explored);
            });

          } else {
            runtime.hero.moves.clear();
          }
        }
        break;

      case ActionType::Mount:
        need_cooldown = update_actor_mount(state.hero());
        break;

      case ActionType::Dismount:
        need_cooldown = update_actor_dismount(state.hero());
        break;

      case ActionType::Reload:
        need_cooldown = update_actor_reload(state.hero());
        break;
    }

    if (check_actor_position(state.hero())) {
      runtime.hero.moves.clear();
    }

    runtime.hero.action = {};
    return need_cooldown;
  }

  bool WorldModel::check_actor_position(ActorState& actor)
  {
    MapCellDecoration decoration = MapCellDecoration::None;

    switch (actor.floor) {
      case Floor::Underground:
        decoration = state.map.underground(actor.position).decoration;
        break;
      case Floor::Ground:
        decoration = state.map.ground(actor.position).decoration;
        break;
      case Floor::Upstairs:
        assert(false); // TODO: upstairs
        break;
    }

    switch (decoration) {
      case MapCellDecoration::FloorDown:
        return change_floor(actor, compute_floor_down(actor.floor));
      case MapCellDecoration::FloorUp:
        return change_floor(actor, compute_floor_up(actor.floor));
      default:
        break;
    }

    return false;
  }

  bool WorldModel::change_floor(ActorState& actor, Floor new_floor)
  {
    gf::Log::debug("Want to change floor: {} -> {}", int(actor.floor), int(new_floor));

    if (actor.floor == new_floor) {
      return false;
    }

    if (actor.feature.type() == ActorType::Human && actor.feature.from<ActorType::Human>().mounting != NoIndex) {
      // actor is mounted, no floor change possible
      return false;
    }

    FloorMap& old_floor_map = runtime.map.from_floor(actor.floor);
    FloorMap& new_floor_map = runtime.map.from_floor(new_floor);

    ReverseMapCell& old_map_cell = old_floor_map.reverse(actor.position);
    ReverseMapCell& new_map_cell = new_floor_map.reverse(actor.position);

    if (new_map_cell.actor_index != NoIndex) {
      return false;
    }

    gf::Log::debug("Change floor!");

    std::swap(old_map_cell.actor_index, new_map_cell.actor_index);
    actor.floor = new_floor;

    // update fov for hero

    if (&actor == &state.hero()) {
      BackgroundMap& map = state.map.from_floor(new_floor);
      gf::compute_symmetric_shadowcasting(map, map, actor.position, HeroVisionRange, [](MapCell& cell) {
        cell.properties.set(MapCellProperty::Visible);
        cell.properties.set(MapCellProperty::Explored);
      });
    }

    return true;
  }

  bool WorldModel::update_actor_mount(ActorState& actor)
  {
    FloorMap& floor_map = runtime.map.from_floor(actor.floor);

    HumanFeature& actor_feature = actor.feature.from<ActorType::Human>();
    ReverseMapCell& actor_cell = floor_map.reverse(actor.position);

    if (actor_feature.mounting != NoIndex) {
      // the hero is already mouting an animal
      return false;
    }

    gf::Log::debug("The hero is not mouting an animal.");

    std::vector<uint32_t> actor_indices;

    for (const gf::Vec2I neighbor : floor_map.reverse.compute_4_neighbors_range(actor.position)) {
      const ReverseMapCell& cell = floor_map.reverse(neighbor);

      if (cell.actor_index == NoIndex) {
        // not actor on this cell
        continue;
      }

      gf::Log::debug("There is an actor next to the hero: {}", cell.actor_index);

      actor_indices.push_back(cell.actor_index);
    }

    for (const uint32_t animal_actor_index : actor_indices) {
      assert(animal_actor_index < state.actors.size());
      ActorState& animal_actor = state.actors[animal_actor_index];

      if (animal_actor.feature.type() != ActorType::Animal) {
        // it's not an animal
        continue;
      }

      gf::Log::debug("The actor {} is an animal", animal_actor_index);

      const AnimalDataFeature& animal_data_feature = animal_actor.data->feature.from<ActorType::Animal>();

      if (!animal_data_feature.can_be_mounted) {
        // the animal cannot be mounted
        continue;
      }

      gf::Log::debug("The actor {} can be mounted", animal_actor_index);

      AnimalFeature& animal_feature = animal_actor.feature.from<ActorType::Animal>();

      if (animal_feature.mounted_by != NoIndex) {
        // the animal is already mounted
        continue;
      }

      gf::Log::debug("Mount!");

      actor_feature.mounting = animal_actor_index;
      actor.position = animal_actor.position;

      std::swap(animal_feature.mounted_by, actor_cell.actor_index);
      update_current_task_in_queue(MountTime);
      break;
    }

    return true;
  }

  bool WorldModel::update_actor_dismount(ActorState& actor)
  {
    FloorMap& floor_map = runtime.map.from_floor(actor.floor);

    HumanFeature& actor_feature = actor.feature.from<ActorType::Human>();

    if (actor_feature.mounting == NoIndex) {
      // the actor is not mouting an animal
      return false;
    }

    gf::Log::debug("The actor is mouting an animal.");

    std::optional<gf::Vec2I> position;

    for (const gf::Vec2I neighbor : floor_map.reverse.compute_4_neighbors_range(actor.position)) {
      if (!is_walkable(actor.floor, neighbor)) {
        continue;
      }

      gf::Log::debug("There is an empty place next to the actor");

      position = neighbor;
      break;
    }

    if (!position) {
      gf::Log::debug("There is no empty place next to the actor");
      return false;
    }

    actor.position = *position;
    ReverseMapCell& actor_cell = floor_map.reverse(actor.position);
    assert(actor_cell.actor_index == NoIndex);
    actor_cell.actor_index = 0;

    ActorState& mount = state.actors[actor_feature.mounting];
    assert(mount.feature.type() == ActorType::Animal);
    mount.feature.from<ActorType::Animal>().mounted_by = NoIndex;

    actor_feature.mounting = NoIndex;
    update_current_task_in_queue(MountTime);
    return true;
  }

  bool WorldModel::update_actor_reload(ActorState& actor)
  {
    if (actor.weapon.data && actor.weapon.data->feature.type() == ItemType::Firearm && actor.ammunition.data && actor.ammunition.data->feature.type() == ItemType::Ammunition) {
      const FirearmDataFeature& firearm = actor.weapon.data->feature.from<ItemType::Firearm>();
      const AmmunitionDataFeature& ammunition = actor.ammunition.data->feature.from<ItemType::Ammunition>();

      if (firearm.caliber == ammunition.caliber) {
        const int8_t needed_cartridges = firearm.capacity - actor.weapon.cartridges;
        const int8_t loaded_cartriges = static_cast<int8_t>(std::min<int16_t>(needed_cartridges, actor.ammunition.count));

        if (loaded_cartriges > 0) {
          actor.weapon.cartridges += loaded_cartriges;
          actor.ammunition.count -= loaded_cartriges;

          state.add_message(fmt::format("<style=character>{}</> reloads its weapon with {} cartridges.", actor.feature.from<ActorType::Human>().name, loaded_cartriges));

          update_current_task_in_queue(firearm.reload_time);
          return true;
        }

      }
    }

    return false;
  }

  bool WorldModel::update_actor(ActorState& actor)
  {
    const int32_t distance_to_hero = gf::chebyshev_distance(actor.position, state.hero().position);

    if (distance_to_hero > IdleDistance) {
      update_current_task_in_queue(static_cast<uint16_t>(distance_to_hero - IdleDistance + IdleTime));
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
    assert(cow.feature.type() == ActorType::Animal);

    if (cow.feature.from<ActorType::Animal>().mounted_by != NoIndex) {
      update_current_task_in_queue(IdleTime);
      return;
    }

    gf::Orientation orientation = random_orientation(m_random);
    gf::Vec2I new_position = cow.position + gf::displacement(orientation);

    int tries = 0;

    for (;;) {
      if (tries == MaxMoveTries) {
        new_position = cow.position;
        break;
      }

      if (is_walkable(cow.floor, new_position) && is_prairie(new_position)) { // TODO: maybe change this, a cow should be able to walk anywhere
        break;
      }

      orientation = random_orientation(m_random);
      new_position = cow.position + gf::displacement(orientation);
      ++tries;
    }

    move_actor(cow, new_position);
    update_current_task_in_queue(GrazeTime);
  }

  bool WorldModel::update_train(TrainState& train, uint32_t train_index)
  {
    runtime.set_reverse_train(train, NoIndex);

    const uint32_t new_index = runtime.network.prev_position(train.railway_index);
    assert(new_index < runtime.network.railway.size());
    const gf::Vec2I new_position = runtime.network.railway[new_index];

    train.railway_index = new_index;

    runtime.set_reverse_train(train, train_index);

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
