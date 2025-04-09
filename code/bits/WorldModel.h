#ifndef FFW_WORLD_MODEL_H
#define FFW_WORLD_MODEL_H

#include <gf2/core/Model.h>

#include "WorldData.h"
#include "WorldRuntime.h"
#include "WorldState.h"

namespace ffw {

  struct WorldModel : gf::Model {
    WorldData data;
    WorldState state;
    WorldRuntime runtime;

    void update(gf::Time time) override;


    bool is_walkable(gf::Vec2I position) const;

    void move_actor(ActorState& actor, gf::Vec2I position);
  };

}

#endif // FFW_WORLD_MODEL_H
