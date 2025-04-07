#include "WorldRuntime.h"

#include "WorldState.h"

namespace ffw {

  void WorldRuntime::bind([[maybe_unused]] const WorldData& data, const WorldState& state)
  {
    map.bind(state.map);
  }

}
