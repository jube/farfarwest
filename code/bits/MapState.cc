#include "MapState.h"

namespace ffw {

  uint32_t NetworkState::next_position(uint32_t current, uint32_t advance) const
  {
    return (current + advance) % railway.size();
  }

  uint32_t NetworkState::prev_position(uint32_t current, uint32_t advance) const
  {
    return (current + railway.size() - advance) % railway.size();
  }

}
