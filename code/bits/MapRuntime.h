#ifndef FFW_MAP_RUNTIME_H
#define FFW_MAP_RUNTIME_H

#include <gf2/core/Console.h>
#include <gf2/core/GridMap.h>

namespace ffw {

  struct MapRuntime {
    gf::Console ground_console;
    gf::GridMap ground_grid;

  };

}

#endif // FFW_MAP_RUNTIME_H
