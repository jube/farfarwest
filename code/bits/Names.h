#ifndef FFW_NAMES_H
#define FFW_NAMES_H

#include <string>

#include <gf2/core/Random.h>

namespace ffw {

  std::string generate_random_male_name(gf::Random* random);
  std::string generate_random_female_name(gf::Random* random);

}


#endif // FFW_NAMES_H
