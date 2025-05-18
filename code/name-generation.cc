#include <gf2/core/Log.h>
#include <gf2/core/Random.h>

#include "bits/Names.h"

int main()
{
  gf::Log::debug("male: {}, female: {}, surname: {}", ffw::compute_max_length(ffw::NameType::MaleName), ffw::compute_max_length(ffw::NameType::FemaleName), ffw::compute_max_length(ffw::NameType::Surname));

  gf::Random random;

  gf::Log::info("Female name: {}", ffw::generate_random_white_female_name(&random));
  gf::Log::info("Male name: {}", ffw::generate_random_white_male_name(&random));
}
