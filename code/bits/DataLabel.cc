#include "DataLabel.h"

namespace ffw {

  void from_json(const nlohmann::json& json, DataLabel& label)
  {
    if (json.is_null()) {
      label.tag = "";
      label.id = gf::InvalidId;
    } else {
      json.get_to(label.tag);
      label.id = gf::hash_string(label.tag);
    }
  }

}
