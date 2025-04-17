#ifndef FFW_NAMES_H
#define FFW_NAMES_H

#include <string>

#include <gf2/core/Random.h>

namespace ffw {

  enum class NameType {
    MaleName,
    FemaleName,
    Surname,
  };

  std::string generate_random_male_name(gf::Random* random);
  std::string generate_random_female_name(gf::Random* random);

  std::size_t compute_max_length(NameType type);

}


#endif // FFW_NAMES_H
