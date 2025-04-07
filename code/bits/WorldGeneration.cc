#include "WorldGeneration.h"
#include "MapState.h"

#include <cstdint>

#include <gf2/core/Array2D.h>
#include <gf2/core/Clock.h>
#include <gf2/core/Heightmap.h>
#include <gf2/core/Log.h>
#include <gf2/core/Noises.h>
#include <gf2/core/ProcGen.h>
#include <gf2/core/Vec2.h>

#include "Settings.h"

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

    constexpr double WorldNoiseScale = WorldBasicSize / 256.0;

    constexpr double AltitudeThreshold = 0.55;
    constexpr double MoistureLoThreshold = 0.45;
    constexpr double MoistureHiThreshold = 0.55;

    constexpr gf::Color PrairieColor = 0xC4D6B0;
    constexpr gf::Color DesertColor = 0xC2B280;
    constexpr gf::Color ForestColor = 0x4A6A4D;
    constexpr gf::Color MountainColor = 0x8B5A2B;

    constexpr float ColorLighterBound = 0.03f;

    constexpr double PrairieHerbProbability = 0.2;
    constexpr double DesertCactusProbability = 0.02;
    constexpr double ForestTreeProbability = 0.25;

    constexpr float MoutainThreshold       = 0.4f;
    constexpr int MoutainSurvivalThreshold = 6;
    constexpr int MoutainBirthThreshold    = 8;
    constexpr int MoutainIterations        = 7;

    /*
     * Outline
     */

    MapState generate_outline(gf::Random* random)
    {
      MapState state = {};
      state.cells = { WorldSize };

      gf::PerlinNoise2D altitude_noise(*random, WorldNoiseScale);
      gf::Heightmap altitude_heightmap(WorldSize);
      altitude_heightmap.add_noise(&altitude_noise);
      altitude_heightmap.normalize();

      gf::PerlinNoise2D moisture_noise(*random, WorldNoiseScale);
      gf::Heightmap moisture_heightmap(WorldSize);
      moisture_heightmap.add_noise(&moisture_noise);
      moisture_heightmap.normalize();

      for (const gf::Vec2I position : state.cells.position_range()) {
        MapCell& cell = state.cells(position);

        MapTrait& trait = cell.trait;
        trait.altitude = altitude_heightmap.value(position);
        trait.moisture = moisture_heightmap.value(position);

        MapDetail& detail = cell.detail;
        detail.state = random->compute_uniform_float(1.0f);
        assert(0.0f <= detail.state && detail.state < 1.0f);

        /*
         *          1 +---------+--------+
         *            | Moutain | Forest |
         *            +--------++--------+
         *            | Desert | Prairie |
         * altitude 0 +--------+---------+
         *            0                  1
         *            moisture
         */

        if (trait.altitude < AltitudeThreshold) {
          if (trait.moisture < MoistureLoThreshold) {
            trait.region = MapRegion::Desert;
            trait.background = DesertColor;

            if (random->compute_bernoulli(DesertCactusProbability * trait.moisture / MoistureLoThreshold)) {
              detail.block = MapBlock::Cactus;
            }
          } else {
            trait.region = MapRegion::Prairie;
            trait.background = PrairieColor;

            if (random->compute_bernoulli(PrairieHerbProbability * trait.moisture)) {
              detail.decoration = MapDecoration::Herb;
            }
          }
        } else {
          if (trait.moisture < MoistureHiThreshold) {
            trait.region = MapRegion::Moutain;
            trait.background = MountainColor;

            // cliffs are put later
          } else {
            trait.region = MapRegion::Forest;
            trait.background = ForestColor;

            if (random->compute_bernoulli(ForestTreeProbability * trait.moisture)) {
              detail.block = MapBlock::Tree;
            }
          }
        }

        trait.background = gf::lighter(trait.background, random->compute_uniform_float(0.0f, ColorLighterBound));
      }

      if constexpr (Debug) {
        gf::Image image(WorldSize);

        for (const gf::Vec2I position : image.position_range()) {
          const MapCell& cell = state.cells(position);
          image.put_pixel(position, cell.trait.background);
        }

        image.save_to_file("00_outline.png");
      }

      return state;
    }

    void generate_mountains(MapState& state, gf::Random* random)
    {
      enum Type : uint8_t {
        Ground,
        Cliff,
      };

      gf::Array2D<Type> map(WorldSize, Ground);

      for (const gf::Vec2I position : state.cells.position_range()) {
        if (state.cells(position).trait.region == MapRegion::Moutain) {
          if (random->compute_bernoulli(MoutainThreshold)) {
            map(position) = Cliff;
          }
        }
      }

      gf::Array2D<Type> next(WorldSize);

      for (int i = 0; i < MoutainIterations; ++i) {
        for (const gf::Vec2I position : map.position_range()) {
          if (state.cells(position).trait.region != MapRegion::Moutain) {
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
        if (state.cells(position).trait.region != MapRegion::Moutain) {
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
          state.cells(position).detail.block = MapBlock::Cliff;
        }
      }
    }



    struct WorldNetwork {
      // cities

      // rails

      // roads

    };

  }

  WorldState generate_world(gf::Random* random)
  {
    gf::Clock clock;

    WorldState state = {};
    state.current_date = Date::generate_random(random);
    gf::Log::info("Generating outline...");
    state.map = generate_outline(random);
    gf::Log::info("Elapsed time: {:g}s", clock.elapsed_time().as_seconds());
    gf::Log::info("Generating moutains...");
    generate_mountains(state.map, random);
    gf::Log::info("Elapsed time: {:g}s", clock.elapsed_time().as_seconds());

    // state.map = generate_map(outline, random);

    ActorState hero = {};
    hero.data = "Hero";
    hero.position = WorldSize / 2;
    state.actors.push_back(hero);

    state.log.messages.push_back({ state.current_date, "Hello <style=character>John</>!" });

    return state;
  }

}
