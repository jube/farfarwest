#include "WorldGeneration.h"

#include <cstdint>

#include <gf2/core/Array2D.h>
#include <gf2/core/Clock.h>
#include <gf2/core/ConsoleChar.h>
#include <gf2/core/Heightmap.h>
#include <gf2/core/Log.h>
#include <gf2/core/Noises.h>
#include <gf2/core/ProcGen.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  namespace {
    /*
     * - world outline
     *   - regions
     *   - color
     *   - natural scenery
     * - rail network
     *   - stations
     * - characters and creatures
     *
     */

    constexpr bool Debug = true;

    constexpr int32_t WorldBasicSize = 4096;
    constexpr gf::Vec2I WorldSize = { WorldBasicSize, WorldBasicSize };

    constexpr double WorldNoiseScale = WorldBasicSize / 256.0;

    constexpr double AltitudeThreshold = 0.55;
    constexpr double MoistureLoThreshold = 0.45;
    constexpr double MoistureHiThreshold = 0.55;

    constexpr gf::Color PrairieColor = 0xC4D6B0;
    constexpr gf::Color DesertColor = 0xC2B280;
    constexpr gf::Color ForestColor = 0x4A6A4D;
    constexpr gf::Color MountainColor = 0x8B5A2B;

    constexpr float ColorLighterBound = 0.03f;

    constexpr double PrairieHerbProbability = 0.1;
    constexpr double DesertCactusProbability = 0.01;
    constexpr double ForestTreeProbability = 0.2;

    constexpr float MoutainThreshold       = 0.4f;
    constexpr int MoutainSurvivalThreshold = 6;
    constexpr int MoutainBirthThreshold    = 8;
    constexpr int MoutainIterations        = 7;

    /*
     * Outline
     */

    enum class WorldRegion : uint8_t {
      Prairie,
      Desert,
      Forest,
      Moutain,
    };

    enum class Block {
      None,
      Cactus,
      Cliff,
      Tree,
    };

    enum Decoration {
      None,
      Herb,
    };

    struct WorldCell {
      double altitude;
      double moisture;
      WorldRegion type = WorldRegion::Prairie;
      gf::Color color;
      Block block = Block::None;
      Decoration decoration = Decoration::None;
    };

    struct WorldOutline {
      gf::Array2D<WorldCell> cells;
    };

    WorldOutline generate_outline(gf::Random* random)
    {
      WorldOutline outline;
      outline.cells = { WorldSize };

      gf::PerlinNoise2D altitude_noise(*random, WorldNoiseScale);
      gf::Heightmap altitude_heightmap(WorldSize);
      altitude_heightmap.add_noise(&altitude_noise);
      altitude_heightmap.normalize();

      gf::PerlinNoise2D moisture_noise(*random, WorldNoiseScale);
      gf::Heightmap moisture_heightmap(WorldSize);
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
            region.color = DesertColor;

            if (random->compute_bernoulli(DesertCactusProbability * region.moisture / MoistureLoThreshold)) {
              region.block = Block::Cactus;
            }
          } else {
            region.type = WorldRegion::Prairie;
            region.color = PrairieColor;

            if (random->compute_bernoulli(PrairieHerbProbability * region.moisture)) {
              region.decoration = Decoration::Herb;
            }
          }
        } else {
          if (region.moisture < MoistureHiThreshold) {
            region.type = WorldRegion::Moutain;
            region.color = MountainColor;

            // cliffs are put later
          } else {
            region.type = WorldRegion::Forest;
            region.color = ForestColor;

            if (random->compute_bernoulli(ForestTreeProbability * region.moisture)) {
              region.block = Block::Tree;
            }
          }
        }

        region.color = gf::lighter(region.color, random->compute_uniform_float(0.0f, ColorLighterBound));
      }

      if constexpr (Debug) {
        gf::Image image(WorldSize);

        for (const gf::Vec2I position : image.position_range()) {
          const WorldCell& region = outline.cells(position);
          image.put_pixel(position, region.color);
        }

        image.save_to_file("00_outline.png");
      }

      return outline;
    }

    void generate_mountains(WorldOutline& outline, gf::Random* random)
    {
      enum Type : uint8_t {
        Ground,
        Cliff,
      };

      gf::Array2D<Type> map(WorldSize, Ground);

      for (const gf::Vec2I position : outline.cells.position_range()) {
        if (outline.cells(position).type == WorldRegion::Moutain) {
          if (random->compute_bernoulli(MoutainThreshold)) {
            map(position) = Cliff;
          }
        }
      }

      gf::Array2D<Type> next(WorldSize);

      for (int i = 0; i < MoutainIterations; ++i) {
        for (const gf::Vec2I position : map.position_range()) {
          if (outline.cells(position).type != WorldRegion::Moutain) {
            continue;
          }

          int count = 0;

          for (const gf::Vec2I neighbor : map.compute_12_neighbors_range(position)) {
            count += map(neighbor) == Ground ? 1 : 0;;
          }

          if (map(position) == Ground) {
            if (count >= MoutainSurvivalThreshold) {
              next(position) = Ground;
            } else {
              next(position) = Cliff;
            }
          } else {
            if (count >= MoutainBirthThreshold) {
              next(position) = Ground;
            } else {
              next(position) = Cliff;
            }
          }
        }

        std::swap(map, next);
      }

      // check for isolated Ground

      for (const gf::Vec2I position : map.position_range()) {
        if (outline.cells(position).type != WorldRegion::Moutain) {
          continue;
        }

        if (map(position) == Ground) {
          bool isolated = true;

          for (gf::Vec2I neighbor : map.compute_4_neighbors_range(position)) {
            if (map.valid(neighbor) && map(neighbor) == Ground) {
              isolated = false;
              break;
            }
          }

          if (isolated) {
            map(position) = Cliff;
          }
        }
      }

      // put in outline

      for (const gf::Vec2I position : map.position_range()) {
        if (map(position) == Cliff) {
          outline.cells(position).block = Block::Cliff;
        }
      }
    }



    struct WorldNetwork {
      // rails

      // roads

    };

    char16_t generate_character(std::initializer_list<char16_t> list, gf::Random* random)
    {
      assert(list.size() > 0);
      const std::size_t index = random->compute_uniform_integer(std::size_t(0), list.size() - 1);
      assert(index < list.size());
      return std::data(list)[index];
    }


    gf::Console generate_primary_map(const WorldOutline& outline, gf::Random* random)
    {
      gf::Console primary_map(WorldSize);

      for (auto position : outline.cells.position_range()) {
        const WorldCell& region = outline.cells(position);
        gf::Color foreground_color = gf::Transparent;
        char16_t character = u' ';

        switch (region.block) {
          case Block::None:
            break;
          case Block::Cactus:
            character = generate_character({ u'!', gf::ConsoleChar::InvertedExclamationMark }, random);
            foreground_color = gf::darker(gf::Green, 0.3f);
            break;
          case Block::Tree:
            character = generate_character({ gf::ConsoleChar::TopHalfIntegral, gf::ConsoleChar::YenSign }, random);
            foreground_color = gf::darker(gf::Green, 0.7f);
            break;
          case Block::Cliff:
            {
              uint8_t neighbor_bits = 0b0000;
              uint8_t direction_bit = 0b0001;

              for (const gf::Orientation orientation : { gf::Orientation::North, gf::Orientation::East, gf::Orientation::South, gf::Orientation::West }) {
                const gf::Vec2I target = position + gf::displacement(orientation);

                if (!outline.cells.valid(target) || outline.cells(target).block == Block::Cliff) {
                  neighbor_bits |= direction_bit;
                }

                direction_bit <<= 1;
              }

              // clang-format off
              constexpr char16_t BlockCharacters[] = {
                                                      // WSEN
                gf::ConsoleChar::FullBlock,           // 0000
                gf::ConsoleChar::UpperHalfBlock,      // 0001
                gf::ConsoleChar::RightHalfBlock,      // 0010
                gf::ConsoleChar::FullBlock,           // 0011 // gf::ConsoleChar::QuadrantUpperRight
                gf::ConsoleChar::LowerHalfBlock,      // 0100
                gf::ConsoleChar::FullBlock,           // 0101
                gf::ConsoleChar::FullBlock,           // 0110 // gf::ConsoleChar::QuadrantLowerRight
                gf::ConsoleChar::FullBlock,           // 0111
                gf::ConsoleChar::LeftHalfBlock,       // 1000
                gf::ConsoleChar::FullBlock,           // 1001 // gf::ConsoleChar::QuadrantUpperLeft
                gf::ConsoleChar::FullBlock,           // 1010
                gf::ConsoleChar::FullBlock,           // 1011
                gf::ConsoleChar::FullBlock,           // 1100 // gf::ConsoleChar::QuadrantLowerLeft
                gf::ConsoleChar::FullBlock,           // 1101
                gf::ConsoleChar::FullBlock,           // 1110
                gf::ConsoleChar::FullBlock,           // 1111
              };
              // clang-format on

              assert(neighbor_bits < std::size(BlockCharacters));
              character = BlockCharacters[neighbor_bits];
              foreground_color = gf::darker(MountainColor, 0.5f);
            }
            break;
        }

        switch (region.decoration) {
          case Decoration::None:
            break;
          case Decoration::Herb:
            character = generate_character({ u',', u'`', u'.', u'\'', gf::ConsoleChar::SquareRoot }, random);
            foreground_color = gf::darker(PrairieColor, 0.3f);
            break;
        }

        primary_map.put_character(position, character, foreground_color, region.color);
      }

      return primary_map;
    }

  }

  WorldState generate_world(gf::Random* random)
  {
    gf::Clock clock;

    gf::Log::info("Generating outline...");
    auto outline = generate_outline(random);
    gf::Log::info("Elapsed time: {:g}s", clock.elapsed_time().as_seconds());

    gf::Log::info("Generating moutains...");
    generate_mountains(outline, random);
    gf::Log::info("Elapsed time: {:g}s", clock.elapsed_time().as_seconds());

    WorldState state = {};
    state.current_date = Date::generate_random(random);

    state.map.primary = generate_primary_map(outline, random);

    state.hero.position = WorldSize / 2;


    state.log.messages.push_back({ state.current_date, "Hello <style=character>John</>!" });

    return state;
  }

}
