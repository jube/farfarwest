#include "bits/FarFarWestSystem.h"

#include "config.h"

int main()
{
  ffw::FarFarWestSystem game(ffw::FarFarWestDataDirectory);
  return game.run();
}
