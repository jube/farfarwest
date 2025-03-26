#include "WorldGeneration.h"

#include <cstdint>

#include <gf2/core/Array2D.h>
#include <gf2/core/Heightmap.h>
#include <gf2/core/Noises.h>
#include <gf2/core/ProcGen.h>
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

    constexpr int32_t WorldOutlineBasicSize = 4096;
    constexpr gf::Vec2I WorldOutlineSize = { WorldOutlineBasicSize, WorldOutlineBasicSize };
    constexpr double WorldOutlineNoiseScale = WorldOutlineBasicSize / 256.0;

    constexpr double AltitudeThreshold = 0.55;
    constexpr double MoistureLoThreshold = 0.45;
    constexpr double MoistureHiThreshold = 0.55;

    /*
     * Outline
     */

    enum class WorldRegion : uint8_t {
      Prairie,
      Desert,
      Forest,
      Moutain,
    };

    struct WorldCell {
      double altitude;
      double moisture;
      WorldRegion type = WorldRegion::Prairie;
    };

    struct WorldOutline {
      gf::Array2D<WorldCell> cells;
    };

    WorldOutline generate_outline(gf::Random* random)
    {
      WorldOutline outline;
      outline.cells = { WorldOutlineSize };

      gf::PerlinNoise2D altitude_noise(*random, WorldOutlineNoiseScale);
      gf::Heightmap altitude_heightmap(WorldOutlineSize);
      altitude_heightmap.add_noise(&altitude_noise);
      altitude_heightmap.normalize();

      gf::PerlinNoise2D moisture_noise(*random, WorldOutlineNoiseScale);
      gf::Heightmap moisture_heightmap(WorldOutlineSize);
      moisture_heightmap.add_noise(&moisture_noise);
      moisture_heightmap.normalize();

      for (const gf::Vec2I position : outline.cells.position_range()) {
        WorldCell& region = outline.cells(position);
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
            region.type = WorldRegion::Desert;
          } else {
            region.type = WorldRegion::Prairie;
          }
        } else {
          if (region.moisture < MoistureHiThreshold) {
            region.type = WorldRegion::Moutain;
          } else {
            region.type = WorldRegion::Forest;
          }
        }
      }

      if constexpr (Debug) {
        gf::Image image(WorldOutlineSize);

        for (const gf::Vec2I position : image.position_range()) {
          const WorldCell& region = outline.cells(position);
          gf::Color color = gf::Black;

          switch (region.type) {
            case WorldRegion::Prairie:
              color = 0xC4D6B0;
              break;
            case WorldRegion::Desert:
              color = 0xC2B280;
              break;
            case WorldRegion::Forest:
              color = 0x4A6A4D;
              break;
            case WorldRegion::Moutain:
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
