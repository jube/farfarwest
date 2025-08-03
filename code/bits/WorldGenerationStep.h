#ifndef FFW_WORLD_GENERATION_STEP_H
#define FFW_WORLD_GENERATION_STEP_H

#include <cstdint>

namespace ffw {

  enum class WorldGenerationStep : uint8_t {
    Start,
    File,
    // loading
    Load,
    // generation
    Date,
    Terrain,
    Biomes,
    Moutains,
    Towns,
    Rails,
    Buildings,
    Regions,
    Underground,
    Hero,
    Actors,
    // binding
    Data,
    MapGround,
    MapUnderground,
    MapRails,
    MapTowns,
    MapMinimap,
    Network,
  };

}

#endif // FFW_WORLD_GENERATION_STEP_H
