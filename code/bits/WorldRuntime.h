#ifndef FFW_WORLD_RUNTIME_H
#define FFW_WORLD_RUNTIME_H

#include "MapRuntime.h"

namespace ffw {
  struct WorldData;
  struct WorldState;

  struct WorldRuntime {

    MapRuntime map;

    void bind(const WorldData& data, const WorldState& state);
  };

}

#endif // FFW_WORLD_RUNTIME_H
