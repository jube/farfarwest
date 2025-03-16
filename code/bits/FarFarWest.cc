#include "FarFarWest.h"

#include "Settings.h"

namespace ffw {

  FarFarWest::FarFarWest()
  : gf::ConsoleSceneManager(ConsoleSize)
  {
    // gf::ConsoleStyle style;
    // style.color.background = gf::Green;
    //
    console().put_character({ 30, 30 }, '@');
  }

}
