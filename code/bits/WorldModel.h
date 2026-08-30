#ifndef FW_WORLD_MODEL_H
#define FW_WORLD_MODEL_H

#include <gf2/core/Model.h>
#include <gf2/core/Random.h>

#include "ActorState.h"
#include "Behavior.h"
#include "WorldData.h"
#include "WorldGenerationStep.h"
#include "WorldRuntime.h"
#include "WorldState.h"

namespace fw {

  struct WorldModel : gf::Model {
    WorldModel(gf::Random* random);

    WorldData data;
    WorldState state;
    WorldRuntime runtime;

    void bind(WorldGenerationAnalysis& analysis);

    void update(gf::Time time) override;
    bool is_running() const { return m_phase == ModelPhase::Running; }

    uint32_t index_of(const ActorState& actor) const;

    bool is_walkable(Floor floor, gf::Vec2I position) const;

    void update_current_task_in_queue(Second seconds);


    bool check() const;

  private:
    gf::Random* m_random = nullptr;

    enum class ModelPhase {
      Running,
      Cooldown,
    };

    ModelPhase m_phase = ModelPhase::Running;
    gf::Time m_cooldown;

    BehaviorManager m_behavior_manager;

    void update_date();
    bool update_hero();
    bool update_train(TrainState& train, uint32_t train_index);

    bool check_human(std::size_t index, const HumanComponent& component) const;
    bool check_animal(std::size_t index, const AnimalComponent& component) const;
  };

}

#endif // FW_WORLD_MODEL_H
