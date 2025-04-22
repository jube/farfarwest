#ifndef FFW_WORLD_RUNTIME_H
#define FFW_WORLD_RUNTIME_H

#include <vector>

#include <gf2/core/Random.h>

#include "HeroRuntime.h"
#include "MapRuntime.h"

namespace ffw {
  struct ActorState;
  struct WorldData;
  struct WorldState;

  struct WorldRuntime {
    gf::Vec2I view_center;
    HeroRuntime hero;
    MapRuntime map;

    std::vector<std::size_t> actors_by_distance;

    void sort_actors_by_distance(const std::vector<ActorState>& actors);

    gf::RectI compute_view() const;

    void bind(const WorldData& data, const WorldState& state, gf::Random* random);
  };

}

#endif // FFW_WORLD_RUNTIME_H
