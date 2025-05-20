#ifndef FFW_COLOR_UTILS_H
#define FFW_COLOR_UTILS_H

#include <cstdint>

#include <string_view>

namespace ffw {

  uint32_t to_rbga(std::string_view raw);

}

#endif // FFW_COLOR_UTILS_H
