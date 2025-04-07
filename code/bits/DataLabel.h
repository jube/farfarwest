#ifndef FFW_DATA_LABEL_H
#define FFW_DATA_LABEL_H

#include <string>

#include <nlohmann/json.hpp>

#include <gf2/core/Id.h>

namespace ffw {

  struct DataLabel {
    std::string tag;
    gf::Id id;

    DataLabel& operator=(std::string other) {
      tag = std::move(other);
      id = gf::hash_string(tag);
      return *this;
    }
  };

  void from_json(const nlohmann::json& json, DataLabel& label);

}

#endif // FFW_DATA_LABEL_H
