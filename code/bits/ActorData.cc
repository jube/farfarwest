#include "ActorData.h"

#include <cassert>
#include <cstdint>

#include <gf2/core/StringUtils.h>

namespace ffw {

  namespace {

    uint32_t to_rbga(const std::string& raw)
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

  void from_json(const nlohmann::json& json, ActorData& data)
  {
    json.at("label").get_to(data.label);

    std::string raw_color;
    json.at("color").get_to(raw_color);
    data.color = to_rbga(raw_color);

    std::string raw_picture;
    json.at("picture").get_to(raw_picture);
    const std::u32string utf32 = gf::to_utf32(raw_picture);
    assert(utf32.size() == 1);
    const char32_t picture = utf32.front();
    assert(picture < 0x10000);
    data.picture = static_cast<char16_t>(picture);
  }

}
