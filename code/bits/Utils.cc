#include "Utils.h"

#include <initializer_list>

#include <gf2/core/Log.h>

namespace ffw {

  gf::Direction undisplacement(gf::Vec2I displacement)
  {
    for (gf::Direction direction : { gf::Direction::Up, gf::Direction::Right, gf::Direction::Down, gf::Direction::Left }) {
      if (gf::displacement(direction) == displacement) {
        return direction;
      }
    }

    gf::Log::debug("WTF! {},{}", displacement.x, displacement.y);
    assert(false);
    return gf::Direction::Center;
  }

}
