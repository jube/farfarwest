#include "WorldModel.h"

#include <cassert>
#include <cstdint>

#include "Action.h"
#include "ActorState.h"
#include "Behavior.h"
#include "Index.h"
#include "MapRuntime.h"
#include "MapState.h"
#include "SchedulerState.h"
#include "WorldGenerationStep.h"

namespace fw {

  namespace {

    constexpr gf::Time Cooldown = gf::milliseconds(20);

  }

  WorldModel::WorldModel(gf::Random* random)
  : m_random(random)
  {
  }

  void WorldModel::bind(WorldGenerationAnalysis& analysis)
  {
    analysis.set_step(WorldGenerationStep::Data);
    state.bind(data);
    runtime.bind(data, state, m_random, analysis);
  }

  void WorldModel::update(gf::Time time)
  {
    if (m_phase == ModelPhase::Cooldown) {
      m_cooldown += time;

      if (m_cooldown > Cooldown) {
        m_cooldown -= Cooldown;
        m_phase = ModelPhase::Running;
      }

      return;
    }

    update_date();

    const gf::RectI view = runtime.compute_view();
    bool need_cooldown = false;

    while (state.current_date == state.scheduler.queue.top().date) {
      if (state.scheduler.is_hero_turn()) {
        if (update_hero()) {
          need_cooldown = true;
        }

        break;
      }

      const Task& current_task = state.scheduler.queue.top();

      assert(current_task.index < state.actors.size());
      // gf::Log::debug("[SCHEDULER] {}: Update actor {}", state.current_date.to_string(), current_task.index);
      ActorState& actor = state.actors[current_task.index];

      const Action action = m_behavior_manager.select_behavior(*this, actor, m_random);
      const ActionResult result = compute_action(*this, actor, action);

      if (result == ActionResult::Success && view.contains(actor.location().position)) {
        need_cooldown = true;
      }

      assert(check());
    }

    if (need_cooldown) {
      m_phase = ModelPhase::Cooldown;
    }
  }

  uint32_t WorldModel::index_of(const ActorState& actor) const
  {
    const std::ptrdiff_t offset = &actor - state.actors.data();
    assert(offset >= 0);
    return static_cast<uint32_t>(offset);
  }

  bool WorldModel::is_walkable(Floor floor, gf::Vec2I position) const
  {
    const FloorMap& floor_map = runtime.map.from_floor(floor);

    if (!floor_map.background.valid(position)) {
      return false;
    }

    if (!floor_map.background(position).walkable()) {
      return false;
    }

    assert(floor_map.reverse.valid(position));
    const ReverseMapCell& cell = floor_map.reverse(position);

    if (!cell.empty()) {
      return false;
    }

    // const BackgroundMap& background_map = state.map.from_floor(floor);
    //
    // if (!fw::is_walkable(background_map(position).decoration)) { // TODO: necessary?
    //   return false;
    // }

    return true;
  }

  void WorldModel::update_date()
  {
    state.current_date = state.scheduler.queue.top().date;
    runtime.phase = state.current_date.phase();
  }

  void WorldModel::update_current_task_in_queue(Second seconds)
  {
    Task task = state.scheduler.queue.top();
    state.scheduler.queue.pop();
    task.date.add_seconds(seconds);

    // gf::Log::debug("\tNext turn: {}", task.date.to_string());

    state.scheduler.queue.push(task);
  }


  bool WorldModel::check() const
  {
    // check actors
    for (auto [ index, actor ] : gf::enumerate(state.actors)) {
      switch (actor.component.type()) {
        case ActorType::None:
        case ActorType::Group:
        case ActorType::Train:
          break;
        case ActorType::Human:
          if (!check_human(index, actor.component.from<ActorType::Human>())) {
            return false;
          }
          break;
        case ActorType::Animal:
          if (!check_animal(index, actor.component.from<ActorType::Animal>())) {
            return false;
          }
          break;
      }
    }

    return true;
  }

  bool WorldModel::update_hero()
  {
    ActorState& hero = state.hero();
    const Location location = hero.location();

    if (!runtime.hero.moves.empty()) {
      runtime.hero.action = make_action<MoveAction>(runtime.hero.moves.back() - location.position);
      runtime.hero.moves.pop_back();
    }

    if (runtime.hero.action.type() == ActionType::None) {
      return false;
    }

    gf::Log::debug("[SCHEDULER] {}: Update hero", state.current_date.to_string());

    const ActionResult result = compute_action(*this, hero, runtime.hero.action);

    if (runtime.hero.action.type() == ActionType::Move) {
      if (result == ActionResult::Success) {
        const Location new_location = hero.location();
        BackgroundMap& state_map = state.map.from_floor(new_location.floor);
        const std::vector<gf::Vec2I> explored = compute_hero_fov(new_location.position, state_map);

        // update minimap thanks to field of view
        FloorMap& runtime_map = runtime.map.from_floor(new_location.floor);
        runtime_map.update_minimap_explored(explored);

        if (new_location.floor != location.floor) {
          runtime.hero.moves.clear();
        }
      } else {
        runtime.hero.moves.clear();
      }

    }

    runtime.hero.action = {};
    return result == ActionResult::Success;
  }

  bool WorldModel::check_human(std::size_t index, const HumanComponent& component) const
  {
    const FloorMap& floor_map = runtime.map.from_floor(component.location.floor);

    if (component.mounting != NoIndex) {
      index = component.mounting;
    }

    if (floor_map.reverse(component.location.position).actor_index != index) {
      gf::Log::debug("HUMAN CHECK FAILED: position = {}, {} ; index = {} ; actor_index = {}", component.location.position.x, component.location.position.y, index, floor_map.reverse(component.location.position).actor_index);
      return false;
    }

    return true;
  }

  bool WorldModel::check_animal(std::size_t index, const AnimalComponent& component) const
  {
    const FloorMap& floor_map = runtime.map.from_floor(component.location.floor);

    if (floor_map.reverse(component.location.position).actor_index != index) {
      gf::Log::debug("ANIMAL CHECK FAILED: position = {}, {} ; index = {} ; actor_index = {}", component.location.position.x, component.location.position.y, index, floor_map.reverse(component.location.position).actor_index);
      return false;
    }

    return true;
  }

}
