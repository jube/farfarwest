#ifndef FFW_NETWORK_RUNTIME_H
#define FFW_NETWORK_RUNTIME_H

#include <vector>

#include <gf2/core/Vec2.h>

namespace ffw {
  struct WorldState;

  struct NetworkRuntime {
    std::vector<gf::Vec2I> railway;

    uint32_t next_position(uint32_t current, uint32_t advance = 1) const;
    uint32_t prev_position(uint32_t current, uint32_t advance = 1) const;

    void bind(const WorldState& state);
  };

}

#endif // FFW_NETWORK_RUNTIME_H
