#ifndef FFW_ACTOR_DATA_H
#define FFW_ACTOR_DATA_H

#include <nlohmann/json.hpp>

#include <gf2/core/Color.h>

#include "DataLabel.h"

namespace ffw {

  struct ActorData {
    DataLabel label;
    gf::Color color;
    char16_t picture;
  };

  void from_json(const nlohmann::json& j, ActorData& data);

}

#endif // FFW_ACTOR_DATA_H
