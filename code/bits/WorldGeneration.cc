#include "WorldGeneration.h"

#include <cstdint>

#include <gf2/core/Array2D.h>
#include <gf2/core/Heightmap.h>
#include <gf2/core/Noises.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  namespace {
    /*
     * - world outline
     * - world details
     * - characters and creatures
     *
     */

    constexpr bool Debug = true;

    constexpr int32_t WorldOutlineBasicSize = 64;
    constexpr gf::Vec2I WorldOutlineSize = { WorldOutlineBasicSize, WorldOutlineBasicSize };
    constexpr double WorldOutlineNoiseScale = WorldOutlineBasicSize / 4.0;

    constexpr double AltitudeThreshold = 0.55;
    constexpr double MoistureLoThreshold = 0.45;
    constexpr double MoistureHiThreshold = 0.55;


    enum class WorldRegionType : uint8_t {
      Prairie,
      Desert,
      Forest,
      Moutain,
    };

    struct WorldOutlineRegion {
      double altitude;
      double moisture;
      WorldRegionType type = WorldRegionType::Prairie;
    };

    struct WorldOutline {
      gf::Array2D<WorldOutlineRegion> regions;
    };

    WorldOutline generate_outline(gf::Random* random)
    {
      WorldOutline outline;
      outline.regions = { WorldOutlineSize };

      gf::PerlinNoise2D altitude_noise(*random, WorldOutlineNoiseScale);
      gf::Heightmap altitude_heightmap(WorldOutlineSize);
      altitude_heightmap.add_noise(&altitude_noise);
      altitude_heightmap.normalize();

      gf::PerlinNoise2D moisture_noise(*random, WorldOutlineNoiseScale);
      gf::Heightmap moisture_heightmap(WorldOutlineSize);
      moisture_heightmap.add_noise(&moisture_noise);
      moisture_heightmap.normalize();

      for (auto position : outline.regions.position_range()) {
        WorldOutlineRegion& region = outline.regions(position);
        region.altitude = altitude_heightmap.value(position);
        region.moisture = moisture_heightmap.value(position);

        /*
         *          1 +---------+--------+
         *            | Moutain | Forest |
         *            +--------++--------+
         *            | Desert | Prairie |
         * altitude 0 +--------+---------+
         *            0                  1
         *            moisture
         */

        if (region.altitude < AltitudeThreshold) {
          if (region.moisture < MoistureLoThreshold) {
            region.type = WorldRegionType::Desert;
          } else {
            region.type = WorldRegionType::Prairie;
          }
        } else {
          if (region.moisture < MoistureHiThreshold) {
            region.type = WorldRegionType::Moutain;
          } else {
            region.type = WorldRegionType::Forest;
          }
        }
      }

      if constexpr (Debug) {
        gf::Image image(WorldOutlineSize);

        for (auto position : outline.regions.position_range()) {
          WorldOutlineRegion& region = outline.regions(position);
          gf::Color color = gf::Black;

          switch (region.type) {
            case WorldRegionType::Prairie:
              color = 0xC4D6B0;
              break;
            case WorldRegionType::Desert:
              color = 0xC2B280;
              break;
            case WorldRegionType::Forest:
              color = 0x4A6A4D;
              break;
            case WorldRegionType::Moutain:
              color = 0x8B5A2B;
              break;
          }

          image.put_pixel(position, color);
        }

        image.save_to_file("00_outline.png");
      }

      return outline;
    }




  }


  WorldState generate_world(gf::Random* random)
  {
    auto outline = generate_outline(random);

    WorldState state = {};

    return state;
  }

}
