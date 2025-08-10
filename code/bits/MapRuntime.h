#ifndef FFW_MAP_RUNTIME_H
#define FFW_MAP_RUNTIME_H

#include <cstdint>

#include <atomic>
#include <array>

#include <gf2/core/Array2D.h>
#include <gf2/core/Console.h>
#include <gf2/core/GridMap.h>
#include <gf2/core/Random.h>

#include "Index.h"
#include "MapFloor.h"
#include "WorldGenerationStep.h"

namespace ffw {
  struct WorldState;

  constexpr std::size_t MinimapCount = 4;

  struct ReverseMapCell {
    uint32_t actor_index = NoIndex;
    uint32_t train_index = NoIndex;

    bool empty() const
    {
      return actor_index == NoIndex && train_index == NoIndex;
    }
  };

  struct Minimap {
    gf::Console console;
    int factor;
  };

  struct FloorMap {
    FloorMap() = default;

    explicit FloorMap(gf::Vec2I size)
    : console(size)
    , grid(gf::GridMap::make_orthogonal(size))
    , reverse(size)
    {
    }

    gf::Console console;
    gf::GridMap grid;
    gf::Array2D<ReverseMapCell> reverse;

    std::array<Minimap, MinimapCount> minimaps;
  };

  struct MapRuntime {
    FloorMap underground;
    FloorMap ground;

    const FloorMap& from_floor(Floor floor) const;
    FloorMap& from_floor(Floor floor);

    void bind(const WorldState& state, gf::Random* random, std::atomic<WorldGenerationStep>& step);

    void bind_ground(const WorldState& state, gf::Random* random);
    void bind_underground(const WorldState& state, gf::Random* random);
    void bind_railway(const WorldState& state);
    void bind_roads(const WorldState& state, gf::Random* random);
    void bind_towns(const WorldState& state, gf::Random* random);
    void bind_reverse(const WorldState& state);

    void bind_minimaps(const WorldState& state);
  };

}

#endif // FFW_MAP_RUNTIME_H
