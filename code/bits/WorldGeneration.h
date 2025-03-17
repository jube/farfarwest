#ifndef FFW_WORLD_GENERATION_H
#define FFW_WORLD_GENERATION_H

#include <gf2/core/Random.h>

#include "WorldState.h"

namespace ffw {


  WorldState generate_world(gf::Random& random);

}

#endif // FFW_WORLD_GENERATION_H
