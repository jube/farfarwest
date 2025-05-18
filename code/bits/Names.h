#ifndef FFW_NAMES_H
#define FFW_NAMES_H

#include <string>

#include <gf2/core/Random.h>

namespace ffw {

  std::string generate_random_white_last_name(gf::Random* random);
  std::string generate_random_white_male_name(gf::Random* random, const std::string& last_name = "");
  std::string generate_random_white_female_name(gf::Random* random, const std::string& last_name = "");
  std::string generate_random_white_non_binary_name(gf::Random* random, const std::string& last_name = "");

  enum class NameType {
    MaleName,
    FemaleName,
    Surname,
  };

  std::size_t compute_max_length(NameType type);

}


#endif // FFW_NAMES_H
