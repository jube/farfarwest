#include "Utils.h"

#include <initializer_list>

#include <gf2/core/Log.h>

namespace ffw {

  char16_t to_uppercase_ascii(char16_t c)
  {
    if (u'a' <= c && c <= u'z') {
      return c - u'a' + u'A';
    }

    return c;
  }

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
