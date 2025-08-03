#ifndef FFW_WORLD_GENERATION_H
#define FFW_WORLD_GENERATION_H

#include <atomic>

#include <gf2/core/Random.h>

#include "WorldState.h"
#include "WorldGenerationStep.h"

namespace ffw {

  WorldState generate_world(gf::Random* random, std::atomic<WorldGenerationStep>& step);

}

#endif // FFW_WORLD_GENERATION_H
