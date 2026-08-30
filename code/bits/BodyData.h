#ifndef FW_BODY_DATA_H
#define FW_BODY_DATA_H

#include <cstdint>

#include <string>

#include <nlohmann/json.hpp>

#include "DateTypes.h"

namespace fw {

  struct BodyData {
    int8_t max_health;
    std::string force;
    std::string dexterity;
    std::string constitution;
    std::string luck;
    Second attack_time;
    std::string attack;
    std::string defense;
  };

  void from_json(const nlohmann::json& json, BodyData& data);

}

#endif // FW_BODY_DATA_H
