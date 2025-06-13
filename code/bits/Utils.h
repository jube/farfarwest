#ifndef FFW_UTILS_H
#define FFW_UTILS_H

#include <gf2/core/Direction.h>

namespace ffw {

  char16_t to_uppercase_ascii(char16_t c);

  gf::Direction undisplacement(gf::Vec2I displacement);

}

#endif // FFW_UTILS_H
