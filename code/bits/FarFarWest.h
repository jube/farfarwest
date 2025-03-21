#ifndef FFW_FAR_FAR_WEST_H
#define FFW_FAR_FAR_WEST_H

#include <gf2/core/ConsoleSceneManager.h>
#include <gf2/core/Random.h>

#include "WorldState.h"

namespace ffw {

  struct FarFarWest : gf::ConsoleSceneManager {
    FarFarWest(gf::Random* random);


  private:
    gf::Random* m_random;
    WorldState m_state;
  };

}

#endif // FFW_FAR_FAR_WEST_H
