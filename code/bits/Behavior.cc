#include "Behavior.h"

#include <cassert>

#include "Action.h"
#include "ActorState.h"
#include "Times.h"
#include "WorldModel.h"

namespace fw {

  namespace {

    /*
     * Helpers
     */

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

    /*
     * Action behaviors
     */

    using BehaviorBase = gf::BehaviorNode<BehaviorBlackboard>;

    // Generic action

    class ActionBehavior : public BehaviorBase {
    public:
      ActionBehavior(Action action)
      : m_action(std::move(action))
      {
      }

      gf::BehaviorStatus process(BehaviorBlackboard& blackboard) const override
      {
        blackboard.action = m_action;
        return gf::BehaviorStatus::Running;
      }

    private:
      Action m_action;
    };

    // Wander

    class WanderBehavior : public BehaviorBase {
    public:

      gf::BehaviorStatus process(BehaviorBlackboard& blackboard) const override
      {
        const gf::Orientation orientation = random_orientation(blackboard.random);
        blackboard.action =  make_action<WanderAction>(gf::displacement(orientation));
        return gf::BehaviorStatus::Running;
      }

    };

    /*
     * Condition behaviors
     */

    // IsMounted

    class IsMountedBehavior : public BehaviorBase {
    public:
      gf::BehaviorStatus process(BehaviorBlackboard& blackboard) const override
      {
        if (blackboard.actor->component.type() != ActorType::Animal) {
          return gf::BehaviorStatus::Failure;
        }

        if (blackboard.actor->component.from<ActorType::Animal>().mounted_by == NoIndex) {
          return gf::BehaviorStatus::Failure;
        }

        return gf::BehaviorStatus::Success;
      }
    };

    namespace bt = gf::behavior;

    auto mountable_animal() {
      return bt::sequence<BehaviorBlackboard>(
        IsMountedBehavior(),
        ActionBehavior(make_action<IdleAction>(IdleTime))
      );
    }

    auto lonely_animal() {
      return bt::selector<BehaviorBlackboard>(
        mountable_animal(),
        WanderBehavior()
      );
    }

    auto train_behavior() {
      return ActionBehavior(make_action<CruiseAction>());
    }

    constexpr int32_t IdleDistance = 100;
  }

  BehaviorManager::BehaviorManager()
  {
    using namespace gf::literals;
    m_trees.emplace("Coyote"_id, lonely_animal());
    m_trees.emplace("Grizzli"_id, lonely_animal());
    m_trees.emplace("Snake"_id, lonely_animal());
    m_trees.emplace("Scorpion"_id, lonely_animal());

    m_trees.emplace("Train"_id, train_behavior());
  }

  Action BehaviorManager::select_behavior(const WorldModel& model, const ActorState& actor, gf::Random* random)
  {
    if (model.index_of(actor) == 0) {
      return model.runtime.hero.action;
    }

    if (actor.component.type() == ActorType::Animal) {
      const AnimalElement& element = actor.data->element.from<ActorType::Animal>();
      const AnimalComponent& component = actor.component.from<ActorType::Animal>();

      if (element.can_idle) {
        const int32_t distance = gf::manhattan_distance(component.location.position, model.state.hero().location().position);

        if (distance > IdleDistance) {
          const Second idle_time = (distance / 2 + random->compute_uniform_integer(distance / 10)) * StraightWalkTime;
          return make_action<IdleAction>(idle_time);
        }
      }
    }

    if (auto iterator = m_trees.find(actor.data->label.id); iterator != m_trees.end()) {
      BehaviorBlackboard blackboard = {
        .model = &model,
        .actor = &actor,
        .random = random,
        .action = {},
      };

      const gf::behavior::AnyBehavior<BehaviorBlackboard>& tree = iterator->second;

      [[maybe_unused]] gf::BehaviorStatus status = tree.process(blackboard);
      assert(status == gf::BehaviorStatus::Running);
      assert(blackboard.action);
      return blackboard.action.value();
    }

    gf::Log::debug("No behavior for '{}'", actor.data->label.tag);
    return make_action<IdleAction>(IdleTime);
  }

}
