#include "FarFarWest.h"

#include "WorldGeneration.h"
#include "Settings.h"

namespace ffw {

  FarFarWest::FarFarWest(gf::Random* random)
  : gf::ConsoleSceneManager(ConsoleSize)
  , m_random(random)
  , m_state(generate_world(m_random))
  {
    // gf::ConsoleStyle style;
    // style.color.background = gf::Capri;
    // console().clear(style);

    console().put_character({ 30, 30 }, '@');
  }

}
