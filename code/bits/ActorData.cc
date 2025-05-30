#include "ActorData.h"

#include <cassert>

#include <gf2/core/StringUtils.h>

#include "ColorUtils.h"

namespace ffw {

  NLOHMANN_JSON_SERIALIZE_ENUM( ActorType, {
    { ActorType::None, nullptr },
    { ActorType::Human, "human" },
    { ActorType::Animal, "animal" },
  })

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

    ActorType type = ActorType::None;

    json.at("type").get_to(type);

    switch (type) {
      case ActorType::None:
        break;
      case ActorType::Human:
        {
          HumanDataFeature feature = {};

          data.feature = feature;
        }
        break;
      case ActorType::Animal:
        {
          AnimalDataFeature feature = {};
          json.at("can_be_mounted").get_to(feature.can_be_mounted);

          data.feature = feature;
        }
        break;
    }

  }

}
