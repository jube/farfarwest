#include <gf2/core/Random.h>

#include "bits/WorldGeneration.h"

int main() {
  gf::Random random;
  ffw::generate_world(&random);
}
