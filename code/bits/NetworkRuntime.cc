#include "NetworkRuntime.h"

namespace ffw {

  uint32_t NetworkRuntime::next_position(uint32_t current, uint32_t advance) const
  {
    return (current + advance) % railway.size();
  }

  uint32_t NetworkRuntime::prev_position(uint32_t current, uint32_t advance) const
  {
    return (current + railway.size() - advance) % railway.size();
  }

  void NetworkRuntime::bind(const WorldState& state)
  {

  }

}
