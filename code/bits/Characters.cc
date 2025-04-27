#include "Characters.h"

#include <cassert>

namespace ffw {

  char16_t rotate(char16_t picture, gf::Direction direction)
  {
    assert(direction != gf::Direction::Center);
    const int8_t index = static_cast<int8_t>(direction);
    assert(0 <= index && index < 4);

    // URDL
    switch (picture) {
      case u'▀':
        return u"▀▐▄▌"[index];
      case u'▐':
        return u"▐▄▌▀"[index];
      case u'▄':
        return u"▄▌▀▐"[index];
      case u'▌':
        return u"▌▀▐▄"[index];
      default:
        break;
    }

    return picture;
  }

}
