#include "WorldRuntime.h"

#include <cassert>

#include <algorithm>
#include <numeric>

#include "WorldState.h"

namespace ffw {

  void WorldRuntime::sort_actors_by_distance(const std::vector<ActorState>& actors)
  {
    assert(!actors.empty());

    if (actors_by_distance.size() != actors.size()) {
      actors_by_distance.resize(actors.size());
      std::iota(actors_by_distance.begin(), actors_by_distance.end(), 0);
    }

    const ActorState& hero = actors.front();

    std::sort(actors_by_distance.begin(), actors_by_distance.end(), [&](std::size_t lhs, std::size_t rhs) {
      assert(lhs < actors.size());
      assert(rhs < actors.size());
      return gf::manhattan_distance(actors[lhs].position, hero.position) < gf::manhattan_distance(actors[rhs].position, hero.position);
    });
  }

  void WorldRuntime::bind([[maybe_unused]] const WorldData& data, const WorldState& state, gf::Random* random)
  {
    map.bind(state, random);
    sort_actors_by_distance(state.actors);
  }

}
