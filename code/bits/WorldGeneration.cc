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

#include "Names.h"
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

    constexpr bool Debug = false;

    constexpr double WorldNoiseScale = WorldBasicSize / 256.0;

    constexpr double AltitudeThreshold = 0.55;
    constexpr double MoistureLoThreshold = 0.45;
    constexpr double MoistureHiThreshold = 0.55;

    constexpr double PrairieHerbProbability = 0.2;
    constexpr double DesertCactusProbability = 0.02;
    constexpr double ForestTreeProbability = 0.25;

    constexpr float MoutainThreshold       = 0.4f;
    constexpr int MoutainSurvivalThreshold = 6;
    constexpr int MoutainBirthThreshold    = 8;
    constexpr int MoutainIterations        = 7;

    struct RawCell {
      double altitude;
      double moisture;
    };

    using RawWorld = gf::Array2D<RawCell>;

    /*
     * Outline
     */

    RawWorld generate_raw(gf::Random* random)
    {
      RawWorld raw(WorldSize);

      gf::PerlinNoise2D altitude_noise(*random, WorldNoiseScale);
      gf::Heightmap altitude_heightmap(WorldSize);
      altitude_heightmap.add_noise(&altitude_noise);
      altitude_heightmap.normalize();

      gf::PerlinNoise2D moisture_noise(*random, WorldNoiseScale);
      gf::Heightmap moisture_heightmap(WorldSize);
      moisture_heightmap.add_noise(&moisture_noise);
      moisture_heightmap.normalize();

      for (const gf::Vec2I position : raw.position_range()) {
        RawCell& cell = raw(position);
        cell.altitude = altitude_heightmap.value(position);
        cell.moisture = moisture_heightmap.value(position);
      }

      return raw;
    }

    MapState generate_outline(const RawWorld& raw, gf::Random* random)
    {
      MapState state = {};
      state.cells = { WorldSize };

      for (const gf::Vec2I position : state.cells.position_range()) {
        MapCell& cell = state.cells(position);
        const RawCell& raw_cell = raw(position);

        /*
         *          1 +---------+--------+
         *            | Moutain | Forest |
         *            +--------++--------+
         *            | Desert | Prairie |
         * altitude 0 +--------+---------+
         *            0                  1
         *            moisture
         */

        if (raw_cell.altitude < AltitudeThreshold) {
          if (raw_cell.moisture < MoistureLoThreshold) {
            cell.region = MapRegion::Desert;

            if (random->compute_bernoulli(DesertCactusProbability * raw_cell.moisture / MoistureLoThreshold)) {
              cell.block = MapBlock::Cactus;
            }
          } else {
            cell.region = MapRegion::Prairie;

            if (random->compute_bernoulli(PrairieHerbProbability * raw_cell.moisture)) {
              cell.decoration = MapDecoration::Herb;
            }
          }
        } else {
          if (raw_cell.moisture < MoistureHiThreshold) {
            cell.region = MapRegion::Moutain;

            // cliffs are put later
          } else {
            cell.region = MapRegion::Forest;

            if (random->compute_bernoulli(ForestTreeProbability * raw_cell.moisture)) {
              cell.block = MapBlock::Tree;
            }
          }
        }

        // trait.background = gf::lighter(trait.background, random->compute_uniform_float(0.0f, ColorLighterBound));
      }

      if constexpr (Debug) {
        gf::Image image(WorldSize);

        for (const gf::Vec2I position : image.position_range()) {
          const MapCell& cell = state.cells(position);
          // image.put_pixel(position, cell.trait.background);
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
        if (state.cells(position).region == MapRegion::Moutain) {
          if (random->compute_bernoulli(MoutainThreshold)) {
            map(position) = Cliff;
          }
        }
      }

      gf::Array2D<Type> next(WorldSize);

      for (int i = 0; i < MoutainIterations; ++i) {
        for (const gf::Vec2I position : map.position_range()) {
          if (state.cells(position).region != MapRegion::Moutain) {
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
        if (state.cells(position).region != MapRegion::Moutain) {
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
          state.cells(position).block = MapBlock::Cliff;
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
    gf::Log::info("Starting generation...");
    RawWorld raw = generate_raw(random);
    gf::Log::info("- raw ({:g}s)", clock.elapsed_time().as_seconds());
    state.map = generate_outline(raw, random);
    gf::Log::info("- outline ({:g}s)", clock.elapsed_time().as_seconds());
    generate_mountains(state.map, random);
    gf::Log::info("- moutains ({:g}s)", clock.elapsed_time().as_seconds());

    // state.map = generate_map(outline, random);

    ActorState hero = {};
    hero.data = "Hero";
    hero.position = WorldSize / 2;
    state.actors.push_back(hero);
    state.scheduler.queue.push({state.current_date, 0});

    ActorState cow = {};
    cow.data = "Cow";
    cow.position = hero.position + gf::dirx(10);
    state.actors.push_back(cow);
    state.scheduler.queue.push({state.current_date + 1, 1});

    const std::string name = random->compute_bernoulli(0.5) ? generate_random_female_name(random) : generate_random_male_name(random);

    state.log.messages.push_back({ state.current_date, fmt::format("Hello <style=character>{}</>!", name) });

    gf::Log::info("- actors ({:g}s)", clock.elapsed_time().as_seconds());

    return state;
  }

}
