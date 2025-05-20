#include "ColorUtils.h"

#include <cassert>

namespace ffw {

  uint32_t to_rbga(std::string_view raw)
  {
    assert(raw.size() == 7);
    assert(raw.front() == '#');

    uint32_t color = 0;

    for (std::size_t i = 1; i < raw.size(); ++i) {
      const char c = raw[i];

      if ('0' <= c && c <= '9') {
        color = color * 0x10 + (c - '0');
      } else if ('A' <= c && c <= 'F') {
        color = color * 0x10 + (c - 'A' + 10);
      } else if ('a' <= c && c <= 'f') {
        color = color * 0x10 + (c - 'a' + 10);
      } else {
        assert(false);
      }
    }

    return color;
  }

}
