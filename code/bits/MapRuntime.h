#ifndef FFW_MAP_RUNTIME_H
#define FFW_MAP_RUNTIME_H

#include <gf2/core/Console.h>
#include <gf2/core/GridMap.h>

namespace ffw {
  struct MapState;

  struct MapRuntime {
    gf::Console outside_ground;
    gf::GridMap outside_grid;

    void bind(const MapState& state);
  };

}

#endif // FFW_MAP_RUNTIME_H
