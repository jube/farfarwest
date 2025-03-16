#ifndef FFW_FAR_FAR_WEST_RESOURCES_H
#define FFW_FAR_FAR_WEST_RESOURCES_H

#include <gf2/core/ConsoleData.h>
#include <gf2/core/ResourceBundle.h>

namespace ffw {
  class FarFarWestSystem;

  struct FarFarWestResources {
    FarFarWestResources();

    gf::ResourceBundle bundle(FarFarWestSystem* game) const;

    gf::ConsoleResource console_resource;
  };

}

#endif // FFW_FAR_FAR_WEST_RESOURCES_H
