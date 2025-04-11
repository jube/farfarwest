#include "WorldGeneration.h"
#include "Colors.h"
#include "MapCell.h"
#include "MapState.h"

#include <cstdint>

#include <queue>

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

    constexpr bool Debug = true;

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

    constexpr std::size_t RegionMinimumSize = 400;

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
      }

      if constexpr (Debug) {
        gf::Image image(WorldSize);

        for (const gf::Vec2I position : image.position_range()) {
          const MapCell& cell = state.cells(position);
          gf::Color color = gf::Transparent;

          switch (cell.region) {
            case MapRegion::Prairie:
              color = PrairieColor;
              break;
            case MapRegion::Desert:
              color = DesertColor;
              break;
            case MapRegion::Forest:
              color = ForestColor;
              break;
            case MapRegion::Moutain:
              color = MountainColor;
              break;
          }

          image.put_pixel(position, color);
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

      if constexpr (Debug) {
        gf::Image image(WorldSize);

        for (const gf::Vec2I position : image.position_range()) {
          const MapCell& cell = state.cells(position);
          gf::Color color = gf::Transparent;

          switch (cell.region) {
            case MapRegion::Prairie:
              color = PrairieColor;
              break;
            case MapRegion::Desert:
              color = cell.block == MapBlock::None ? DesertColor : gf::darker(gf::Green, 0.3f);
              break;
            case MapRegion::Forest:
              color = cell.block == MapBlock::None ? ForestColor : gf::darker(gf::Green, 0.7f);
              break;
            case MapRegion::Moutain:
              color = cell.block == MapBlock::None ? MountainColor : gf::darker(MountainColor, 0.5f);;
              break;
          }

          image.put_pixel(position, color);
        }

        image.save_to_file("01_blocks.png");
      }

    }

    struct WorldRegion {
      std::vector<gf::Vec2I> points;
      gf::RectI bounds;
    };

    struct WorldRegions {
      std::vector<WorldRegion> prairie_regions;
      std::vector<WorldRegion> desert_regions;
      std::vector<WorldRegion> forest_regions;
      std::vector<WorldRegion> mountain_regions;

      std::vector<WorldRegion>& operator()(MapRegion region)
      {
        switch (region) {
          case MapRegion::Prairie:
            return prairie_regions;
          case MapRegion::Desert:
            return desert_regions;
          case MapRegion::Forest:
            return forest_regions;
          case MapRegion::Moutain:
            return mountain_regions;
        }

        assert(false);
        return prairie_regions;
      }
    };

    WorldRegions compute_regions(const MapState& state)
    {
      enum class Status : uint8_t {
        New,
        Visited,
      };

      gf::Array2D<Status> status(WorldSize, Status::New);

      WorldRegions regions = {};

      for (const gf::Vec2I position : state.cells.position_range()) {
        if (status(position) == Status::Visited) {
          continue;
        }

        const MapRegion region_type = state.cells(position).region;

        std::queue<gf::Vec2I> queue;
        queue.emplace(position);
        status(position) = Status::Visited;

        WorldRegion region;

        while (!queue.empty()) {
          const gf::Vec2I current = queue.front();
          queue.pop();
          assert(status(current) == Status::Visited);
          region.points.push_back(current);

          for (const gf::Vec2I neighbor : state.cells.compute_4_neighbors_range(current)) {
            if (status(neighbor) == Status::Visited) {
              continue;
            }

            if (state.cells(neighbor).region != region_type) {
              continue;
            }

            status(neighbor) = Status::Visited;
            queue.push(neighbor);
          }
        }

        if (region.points.size() > RegionMinimumSize) {
          regions(region_type).push_back(std::move(region));
        }
      }

      // compute extents

      auto compute_extent = [](std::vector<WorldRegion>& regions, std::string_view name) {

        for (WorldRegion& region : regions) {
          gf::RectI bounds = gf::RectI::from_center_size(region.points.front(), { 1, 1 });

          for (gf::Vec2I point : region.points) {
            bounds.extend_to(point);
          }

          region.bounds = bounds;
        }

        std::sort(regions.begin(), regions.end(), [](const WorldRegion& lhs, const WorldRegion& rhs) {
          return lhs.points.size() > rhs.points.size();
        });

        gf::Log::info("\t{} ({})", name, regions.size());

        for (WorldRegion& region : regions) {
          gf::Log::info("\t\t- Size: {}, Extent: {}x{}, Density: {:g}", region.points.size(), region.bounds.extent.w, region.bounds.extent.h, double(region.points.size()) / double(region.bounds.extent.w * region.bounds.extent.h));
        }
      };

      compute_extent(regions.prairie_regions, "Prairie");
      compute_extent(regions.desert_regions, "Desert");
      compute_extent(regions.forest_regions, "Forest");
      compute_extent(regions.mountain_regions, "Moutain");

      return regions;
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
    compute_regions(state.map);
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
