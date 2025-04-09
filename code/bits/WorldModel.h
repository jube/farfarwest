#ifndef FFW_WORLD_MODEL_H
#define FFW_WORLD_MODEL_H

#include <gf2/core/Model.h>
#include <gf2/core/Random.h>

#include "ActorState.h"
#include "WorldData.h"
#include "WorldRuntime.h"
#include "WorldState.h"

namespace ffw {

  struct WorldModel : gf::Model {
    WorldModel(gf::Random* random);

    WorldData data;
    WorldState state;
    WorldRuntime runtime;

    void update(gf::Time time) override;

    bool is_prairie(gf::Vec2I position) const;

    bool is_walkable(gf::Vec2I position) const;
    void move_actor(ActorState& actor, gf::Vec2I position);

  private:
    gf::Random* m_random = nullptr;

    enum class Phase {
      Running,
      Cooldown,
    };

    Phase m_phase = Phase::Running;
    gf::Time m_cooldown;

    void update_current_actor_in_queue(uint16_t seconds);
    bool update_hero();

    void update_cow(ActorState& cow);
  };

}

#endif // FFW_WORLD_MODEL_H
