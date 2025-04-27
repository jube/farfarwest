#ifndef FFW_WORLD_RUNTIME_H
#define FFW_WORLD_RUNTIME_H

#include <vector>

#include <gf2/core/Random.h>

#include "HeroRuntime.h"
#include "MapRuntime.h"
#include "NetworkRuntime.h"

namespace ffw {
  struct ActorState;
  struct TrainState;
  struct WorldData;
  struct WorldState;

  struct WorldRuntime {
    gf::Vec2I view_center;
    HeroRuntime hero;
    MapRuntime map;
    NetworkRuntime network;

    std::vector<std::size_t> actors_by_distance;

    void sort_actors_by_distance(const std::vector<ActorState>& actors);

    gf::RectI compute_view() const;

    void set_reverse_train(const TrainState& train, uint32_t train_index);

    void bind(const WorldData& data, const WorldState& state, gf::Random* random);

    void bind_network(const WorldState& state);
    void bind_train(const WorldState& state);
  };

}

#endif // FFW_WORLD_RUNTIME_H
