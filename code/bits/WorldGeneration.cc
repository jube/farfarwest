#include "WorldGeneration.h"

#include <gf2/core/Array2D.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  namespace {
    /*
     * - world outline
     * - world details
     * - characters and creatures
     *
     */

    constexpr gf::Vec2 WorldOutlineSize = { 64, 64 };


    enum class WorldOutlineRegionType {
      Plain,
      Moutain,
    };

    struct WorldOutlineRegion {
      double input = 0.0;
      WorldOutlineRegionType type = WorldOutlineRegionType::Plain;
    };

    struct WorldOutline {
      gf::Array2D<WorldOutlineRegion> regions;
    };

    WorldOutline generate_outline(gf::Random& random)
    {
      WorldOutline outline;
      outline.regions = { WorldOutlineSize };

      for (auto position : outline.regions.position_range()) {
        WorldOutlineRegion& region = outline.regions(position);

        region.type = WorldOutlineRegionType::Plain;
      }

      return outline;
    }


  }


  WorldState generate_world(gf::Random& random)
  {
    WorldState state = {};


    return state;
  }

}
