#ifndef FFW_WORLD_RUNTIME_H
#define FFW_WORLD_RUNTIME_H

#include <vector>

#include "MapRuntime.h"

namespace ffw {
  struct ActorState;
  struct WorldData;
  struct WorldState;

  struct WorldRuntime {
    gf::Vec2I orientation = { 0, 0 }; // TODO: put in a HeroRuntime later

    MapRuntime map;

    std::vector<std::size_t> actors_by_distance;

    void sort_actors_by_distance(const std::vector<ActorState>& actors);

    void bind(const WorldData& data, const WorldState& state);
  };

}

#endif // FFW_WORLD_RUNTIME_H
