#ifndef FFW_WORLD_RUNTIME_H
#define FFW_WORLD_RUNTIME_H

#include <vector>

#include "HeroRuntime.h"
#include "MapRuntime.h"

namespace ffw {
  struct ActorState;
  struct WorldData;
  struct WorldState;

  struct WorldRuntime {
    HeroRuntime hero;
    MapRuntime map;

    std::vector<std::size_t> actors_by_distance;

    void sort_actors_by_distance(const std::vector<ActorState>& actors);

    void bind(const WorldData& data, const WorldState& state);
  };

}

#endif // FFW_WORLD_RUNTIME_H
