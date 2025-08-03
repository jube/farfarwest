#include <atomic>

#include <gf2/core/Log.h>
#include <gf2/core/Random.h>

#include "bits/WorldGeneration.h"
#include "bits/WorldGenerationStep.h"

int main() {
  gf::Random random;
  std::atomic<ffw::WorldGenerationStep> step(ffw::WorldGenerationStep::Start);
  ffw::generate_world(&random, step);
}
