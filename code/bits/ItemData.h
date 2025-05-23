#ifndef FFW_ITEM_DATA_H
#define FFW_ITEM_DATA_H

#include <cstdint>

#include <nlohmann/json.hpp>

#include <gf2/core/Color.h>
#include <gf2/core/TaggedVariant.h>

#include "DataLabel.h"

namespace ffw {

  enum class ItemType {
    None,
    Firearm,
    Ammunition,
  };

  struct FirearmDataFeature {
    int8_t caliber;
    int8_t capacity;
    uint16_t reload_time;
  };

  struct AmmunitionDataFeature {
    int8_t caliber;
  };

  using ItemDataFeature = gf::TaggedVariant<ItemType, FirearmDataFeature, AmmunitionDataFeature>;

  struct ItemData {
    DataLabel label;
    gf::Color color;
    char16_t picture;
    ItemDataFeature feature;
  };

  void from_json(const nlohmann::json& j, ItemData& data);

}

#endif // FFW_ITEM_DATA_H
