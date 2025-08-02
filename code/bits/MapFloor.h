#ifndef FFW_MAP_FLOOR_H
#define FFW_MAP_FLOOR_H

#include <cstdint>

namespace ffw {

  enum class Floor : int8_t {
    Underground = -1,
    Ground = 0,
    Upstairs = 1,
  };

}

#endif // FFW_MAP_FLOOR_H
