#include "ActorData.h"

#include <cassert>

#include <gf2/core/StringUtils.h>

#include "ColorUtils.h"

namespace ffw {

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
