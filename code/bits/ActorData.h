#ifndef FFW_ACTOR_DATA_H
#define FFW_ACTOR_DATA_H

#include <nlohmann/json.hpp>

#include <gf2/core/Color.h>
#include <gf2/core/TaggedVariant.h>

#include "DataLabel.h"

namespace ffw {

  enum class ActorType : uint16_t {
    None,
    Human,
    Animal,
  };

  struct HumanDataFeature {

  };

  struct AnimalDataFeature {
    bool can_be_mounted;
  };

  using ActorDataFeature = gf::TaggedVariant<ActorType, HumanDataFeature, AnimalDataFeature>;

  struct ActorData {
    DataLabel label;
    gf::Color color;
    char16_t picture;
    ActorDataFeature feature;
  };

  void from_json(const nlohmann::json& j, ActorData& data);

}

#endif // FFW_ACTOR_DATA_H
