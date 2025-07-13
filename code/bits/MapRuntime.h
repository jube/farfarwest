#ifndef FFW_MAP_RUNTIME_H
#define FFW_MAP_RUNTIME_H

#include <cstdint>

#include <array>
#include <limits>

#include <gf2/core/Array2D.h>
#include <gf2/core/Console.h>
#include <gf2/core/GridMap.h>
#include <gf2/core/Random.h>

namespace ffw {
  struct WorldState;

  constexpr uint32_t NoIndex = std::numeric_limits<uint32_t>::max();

  constexpr std::size_t MinimapCount = 4;

  struct ReverseMapCell {
    uint32_t actor_index = NoIndex;
    uint32_t train_index = NoIndex;

    bool empty() const
    {
      return actor_index == NoIndex && train_index == NoIndex;
    }
  };

  enum Level {
    Underground,
    Ground,
    Floor,
  };

  struct LevelMap {
    LevelMap() = default;

    explicit LevelMap(gf::Vec2I size)
    : console(size)
    , grid(gf::GridMap::make_orthogonal(size))
    , reverse(size)
    {
    }

    gf::Console console;
    gf::GridMap grid;
    gf::Array2D<ReverseMapCell> reverse;
  };

  struct Minimap {
    gf::Console console;
    int factor;
  };

  struct MapRuntime {
    LevelMap ground;

    std::array<Minimap, MinimapCount> minimaps;

    void bind(const WorldState& state, gf::Random* random);

    void bind_ground(const WorldState& state, gf::Random* random);
    void bind_railway(const WorldState& state);
    void bind_towns(const WorldState& state, gf::Random* random);
    void bind_reverse(const WorldState& state);

    void bind_minimaps(const WorldState& state);
  };

}

#endif // FFW_MAP_RUNTIME_H
