#include "WorldGeneration.h"
#include "Colors.h"
#include "MapCell.h"
#include "MapState.h"

#include <algorithm>
#include <cstdint>

#include <queue>

#include <gf2/core/Array2D.h>
#include <gf2/core/Clock.h>
#include <gf2/core/Direction.h>
#include <gf2/core/Easing.h>
#include <gf2/core/GridMap.h>
#include <gf2/core/Heightmap.h>
#include <gf2/core/Log.h>
#include <gf2/core/Noises.h>
#include <gf2/core/ProcGen.h>
#include <gf2/core/Vec2.h>
#include <string_view>

#include "Names.h"
#include "Settings.h"

namespace ffw {

  namespace {

    constexpr bool Debug = true;

    constexpr double WorldNoiseScale = WorldBasicSize / 256.0;

    constexpr int32_t WorldPaddingSize = 150;

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

    constexpr std::size_t TownsCount = 5;
    constexpr int32_t TownRadius = 40;
    constexpr int32_t TownMinDistanceFromOther = 1500;
    constexpr std::size_t FarmsCount = TownsCount * 5;
    constexpr int32_t FarmRadius = 10;
    constexpr int32_t FarmMinDistanceFromOther = 200;

    constexpr std::size_t DayTime = 24 * 60 * 60;
    constexpr int32_t RailSpacing = 2;

    constexpr float SlopeFactor = 225.0f;

    constexpr std::size_t RegionMinimumSize = 400;

    constexpr gf::Vec2I EightNeighbors[] = {
      // clang-format off
      { -1, -1 }, {  0, -1 }, { +1, -1 },
      { -1,  0 },             { +1,  0 },
      { -1, +1 }, {  0, +1 }, { +1, +1 },
      // clang-format on
    };




    bool is_on_side(gf::Vec2I position)
    {
      return position.x == 0 || position.x == WorldBasicSize - 1 || position.y == 0 || position.y == WorldBasicSize - 1;
    }

    enum class ImageType : uint8_t {
      Basic,
      Blocks,
    };

    gf::Image compute_basic_image(const MapState& state, ImageType type = ImageType::Basic) {
      gf::Image image(WorldSize);

      for (const gf::Vec2I position : image.position_range()) {
        const MapCell& cell = state.cells(position);
        gf::Color color = gf::Transparent;

        switch (cell.region) {
          case MapRegion::Prairie:
            color = PrairieColor;
            break;
          case MapRegion::Desert:
            color = type == ImageType::Basic || cell.block == MapBlock::None ? DesertColor : gf::darker(gf::Green, 0.3f);
            break;
          case MapRegion::Forest:
            color = type == ImageType::Basic || cell.block == MapBlock::None ? ForestColor : gf::darker(gf::Green, 0.7f);
            break;
          case MapRegion::Moutain:
            color = type == ImageType::Basic || cell.block == MapBlock::None ? MountainColor : gf::darker(MountainColor, 0.5f);;
            break;
        }

        image.put_pixel(position, color);
      }

      return image;
    }
    /*
     * Step 1. Generate a raw map.
     *
     * The raw map is just the combination of two Perlin noises. One for
     * altitude and one for moisture.
     *
     * The values are used many times in the process but are not kept in the
     * final state of the game.
     */

    struct RawCell {
      double altitude;
      double moisture;
    };

    using RawWorld = gf::Array2D<RawCell>;

    RawWorld generate_raw(gf::Random* random)
    {
      RawWorld raw(WorldSize);

      gf::PerlinNoise2D altitude_noise(random, WorldNoiseScale);
      gf::Heightmap altitude_heightmap(WorldSize);
      altitude_heightmap.add_noise(&altitude_noise);
      altitude_heightmap.normalize();

      gf::PerlinNoise2D moisture_noise(random, WorldNoiseScale);
      gf::Heightmap moisture_heightmap(WorldSize);
      moisture_heightmap.add_noise(&moisture_noise);
      moisture_heightmap.normalize();

      for (const gf::Vec2I position : raw.position_range()) {
        RawCell& cell = raw(position);
        cell.altitude = altitude_heightmap.value(position);
        cell.moisture = moisture_heightmap.value(position);

        double factor = 1.0;

        if (position.x < WorldPaddingSize) {
          factor *= double(position.x) / double (WorldPaddingSize);
        } else if (position.x >= WorldSize.x - WorldPaddingSize) {
          factor *= double(WorldSize.x - position.x - 1) / double (WorldPaddingSize);
        }

        if (position.y < WorldPaddingSize) {
          factor *= double(position.y) / double (WorldPaddingSize);
        } else if (position.y >= WorldSize.y - WorldPaddingSize) {
          factor *= double(WorldSize.y - 1 - position.y) / double (WorldPaddingSize);
        }

        cell.altitude = 1.0 - (1.0 - cell.altitude) * gf::ease_out_cubic(factor);
      }

      return raw;
    }

    /*
     * Step 1. Generate an outline
     *
     * The outline fix the biomes of the map thanks to altitude and moisture
     * values. The constraint is that there must be no desert next to a forest.
     *
     * In this step, simple blocks and decorations are added: trees, cactuses,
     * herbs.
     */

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

            if (is_on_side(position) || random->compute_bernoulli(ForestTreeProbability * raw_cell.moisture)) {
              cell.block = MapBlock::Tree;
            }
          }
        }
      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state);
        image.save_to_file("00_outline.png");
      }

      return state;
    }

    /*
     * Step 2. Generate the moutains
     *
     * The moutains are generated from a cellular automaton. Once it's done,
     * all the blocks are in place.
     */

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

      /*
       * +-+-+-+-+-+
       * | | |X| | |
       * +-+-+-+-+-+
       * | |X|X|X| |
       * +-+-+-+-+-+
       * |X|X|P|X|X|
       * +-+-+-+-+-+
       * | |X|X|X| |
       * +-+-+-+-+-+
       * | | |X| | |
       * +-+-+-+-+-+
       */


      static constexpr gf::Vec2I TwelveNeighbors[] = {
        // clang-format off
                                {  0, -2 },
                    { -1, -1 }, {  0, -1 }, { +1, -1 },
        { -2,  0 }, { -1,  0 },             { +1,  0 }, { +2, 0 },
                    { -1, +1 }, {  0, +1 }, { +1, +1 },
                                {  0, +2 },
        // clang-format on
      };

      for (int i = 0; i < MoutainIterations; ++i) {
        for (const gf::Vec2I position : map.position_range()) {
          if (state.cells(position).region != MapRegion::Moutain) {
            continue;
          }

          int count = 0;

          for (const gf::Vec2I relative_neighbor : TwelveNeighbors) {
            const gf::Vec2I neighbor = position + relative_neighbor;

            if (map.valid(neighbor)) {
              count += map(neighbor) == Ground ? 1 : 0;
            }
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

      constexpr gf::Vec2I FourNeighbors[] = {
        { 0, -1 }, { -1, 0 }, { +1, 0 }, { 0, +1 }
      };

      for (const gf::Vec2I position : map.position_range()) {
        if (state.cells(position).region != MapRegion::Moutain) {
          continue;
        }

        if (map(position) == Ground) {
          bool isolated = true;

          for (const gf::Vec2I relative_neighbor : FourNeighbors) {
            const gf::Vec2I neighbor = position + relative_neighbor;

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
        } else if (state.cells(position).region == MapRegion::Moutain && is_on_side(position)) {
          state.cells(position).block = MapBlock::Cliff;
        }
      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state, ImageType::Blocks);
        image.save_to_file("01_blocks.png");
      }

    }


    /*
     * Step 3. Generate towns and farms.
     *
     */

    struct Town {
      gf::Vec2I center;
      gf::Vec2I rail_arrival;
      gf::Vec2I rail_departure;
    };

    struct WorldPlaces {
      std::array<Town, TownsCount> towns;
      std::array<gf::Vec2I, FarmsCount> farms;

      int32_t min_distance_between_towns() const
      {
        int32_t min_distance = std::numeric_limits<int32_t>::max();

        for (std::size_t i = 0; i < TownsCount; ++i) {
          for (std::size_t j = i + 1; j < TownsCount; ++j) {
            int32_t distance = gf::manhattan_distance(towns[i].center, towns[j].center);
            min_distance = std::min(min_distance, distance);
          }
        }

        return min_distance;
      }

      int32_t min_distance_between_towns_and_farms() const
      {
        int32_t min_distance = std::numeric_limits<int32_t>::max();

        for (std::size_t i = 0; i < FarmsCount; ++i) {
          for (std::size_t j = i + 1; j < FarmsCount; ++j) {
            int32_t distance = gf::manhattan_distance(farms[i], farms[j]);
            min_distance = std::min(min_distance, distance);
          }

          for (std::size_t j = 0; j < TownsCount; ++j) {
            int32_t distance = gf::manhattan_distance(farms[i], towns[j].center);
            min_distance = std::min(min_distance, distance);
          }
        }

        return min_distance;
      }

    };

    bool can_have_place(const MapState& state, gf::Vec2I position, int32_t radius)
    {
      assert(state.cells.valid(position));

      if (state.cells(position).region != MapRegion::Prairie) {
        return false;
      }

      for (const gf::Vec2I neighbor : state.cells.square_range(position, radius)) {
        if (!state.cells.valid(neighbor) || state.cells(neighbor).region != MapRegion::Prairie) {
          return false;
        }
      }

      return true;
    }

    WorldPlaces generate_places(const MapState& state, gf::Random* random)
    {
      WorldPlaces places = {};

      // first generate towns

      int town_rounds = 0;

      for (;;) {
        [[maybe_unused]] int tries = 0;

        for (Town& town : places.towns) {
          do {
            town.center = random->compute_position(gf::RectI::from_size(WorldSize));
            ++tries;
          } while (!can_have_place(state, town.center, TownRadius + RailSpacing));
        }

        const int32_t min_distance = places.min_distance_between_towns();
        // gf::Log::info("Found potential towns after {} tries with min distance {}", tries, min_distance);

        ++town_rounds;

        if (min_distance > TownMinDistanceFromOther) {
          break;
        }
      }

      gf::Log::info("Towns generated after {} rounds", town_rounds);

      // compute rail arrival/departure

      for (Town& town : places.towns) {
        const gf::RectI town_space = gf::RectI::from_center_size(town.center, { 2 * TownRadius + 1, 2 * TownRadius + 1 });
        const gf::Direction direction = gf::direction(gf::angle<float>(WorldCenter - town.center));

        auto put_at = [&](gf::Orientation orientation) {
          return town_space.position_at(orientation) + RailSpacing * gf::displacement(orientation);
        };

        switch (direction) {
          case gf::Direction::Up:
            town.rail_arrival = put_at(gf::Orientation::NorthEast);
            town.rail_departure = put_at(gf::Orientation::NorthWest);
            break;
          case gf::Direction::Right:
            town.rail_arrival = put_at(gf::Orientation::SouthEast);
            town.rail_departure = put_at(gf::Orientation::NorthEast);
            break;
          case gf::Direction::Down:
            town.rail_arrival = put_at(gf::Orientation::SouthWest);
            town.rail_departure = put_at(gf::Orientation::SouthEast);
            break;
          case gf::Direction::Left:
            town.rail_arrival = put_at(gf::Orientation::NorthWest);
            town.rail_departure = put_at(gf::Orientation::SouthWest);
            break;
          case gf::Direction::Center:
            assert(false);
            break;
        }

      }

      // second generate farms

      int farm_rounds = 0;

      for (;;) {
        [[maybe_unused]] int tries = 0;

        for (gf::Vec2I& farm_position : places.farms) {
          do {
            farm_position = random->compute_position(gf::RectI::from_size(WorldSize));
            ++tries;
          } while (!can_have_place(state, farm_position, FarmRadius));
        }

        const int32_t min_distance = places.min_distance_between_towns_and_farms();
        // gf::Log::info("Found potential farms after {} tries with min distance {}", tries, min_distance);

        ++farm_rounds;

        if (min_distance > FarmMinDistanceFromOther) {
          break;
        }
      }

      gf::Log::info("Farms generated after {} rounds", farm_rounds);

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state, ImageType::Blocks);

        for (const Town& town : places.towns) {
          const gf::RectI town_space = gf::RectI::from_center_size(town.center, { 2 * TownRadius + 1, 2 * TownRadius + 1 });

          for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
            image.put_pixel(position, gf::Azure);
          }
        }

        for (const gf::Vec2I farm : places.farms) {
          const gf::RectI farm_space = gf::RectI::from_center_size(farm, { 2 * FarmRadius + 1, 2 * FarmRadius + 1 });

          for (const gf::Vec2I position : gf::rectangle_range(farm_space)) {
            image.put_pixel(position, gf::Azure);
          }
        }

        image.save_to_file("02_places.png");
      }

      return places;
    }

    /*
     * Step 4. Generate railway
     */

    NetworkState generate_network(const RawWorld& raw, MapState& state, const WorldPlaces& places, gf::Random* random)
    {
      // initialize the grid

      gf::GridMap grid = gf::GridMap::make_orthogonal(WorldSize);

      for (const gf::Vec2I position : state.cells.position_range()) {
        const MapCell& cell  = state.cells(position);

        if (cell.block == MapBlock::Cliff) {
          for (const gf::Vec2I neighbor : state.cells.compute_8_neighbors_range(position)) {
            grid.set_walkable(neighbor, false);
          }
        }
      }

      for (const Town& town : places.towns) {
        const gf::RectI town_space = gf::RectI::from_center_size(town.center, { 2 * TownRadius + 1, 2 * TownRadius + 1 });

        for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
          grid.set_walkable(position, false);
        }

        const gf::Vec2I step = gf::sign(town.rail_departure - town.rail_arrival);

        for (gf::Vec2I position = town.rail_arrival + step; position != town.rail_departure; position += step) {
          grid.set_walkable(position, false);
        }
      }

      for (const gf::Vec2I farm : places.farms) {
        const gf::RectI farm_space = gf::RectI::from_center_size(farm, { 2 * FarmRadius + 1, 2 * FarmRadius + 1 }).grow_by(1);

        for (const gf::Vec2I position : gf::rectangle_range(farm_space)) {
          grid.set_walkable(position, false);
        }
      }

      std::vector<std::vector<gf::Vec2I>> paths;

      std::vector<std::size_t> ordered_towns(places.towns.size());
      std::iota(ordered_towns.begin(), ordered_towns.end(), 0);
      std::sort(ordered_towns.begin(), ordered_towns.end(), [&](std::size_t lhs, std::size_t rhs) {
        return gf::angle<float>(places.towns[lhs].center - WorldCenter) < gf::angle<float>(places.towns[rhs].center - WorldCenter);
      });

      for (std::size_t i = 0; i < ordered_towns.size(); ++i) {
        // path for the station

        std::vector<gf::Vec2I> station;
        const gf::Vec2I step = gf::sign(places.towns[ordered_towns[i]].rail_departure - places.towns[ordered_towns[i]].rail_arrival);

        for (gf::Vec2I position = places.towns[ordered_towns[i]].rail_arrival + step; position != places.towns[ordered_towns[i]].rail_departure; position += step) {
          station.push_back(position);
        }

        paths.push_back(std::move(station));

        // path to the next town

        const std::size_t j = (i + 1) % ordered_towns.size();
        auto path = grid.compute_route(places.towns[ordered_towns[i]].rail_departure, places.towns[ordered_towns[j]].rail_arrival, [&](gf::Vec2I position, gf::Vec2I neighbor) {
          const float distance = gf::euclidean_distance<float>(position, neighbor);
          const float slope = std::abs(raw(position).altitude - raw(neighbor).altitude) / distance;
          return distance * (1 + SlopeFactor * gf::square(slope));
        });

        for (const gf::Vec2I point : path) {
          grid.set_walkable(point, false);

          for (const gf::Vec2I relative_neighbor : EightNeighbors) {
            grid.set_walkable(point + relative_neighbor, false);
          }
        }

        assert(!path.empty());
        gf::Log::info("Points between {} and {}: {}", ordered_towns[i], ordered_towns[j], path.size());

        paths.push_back(std::move(path));
      }

      NetworkState network = {};

      for (const std::vector<gf::Vec2I>& path : paths) {
        for (const gf::Vec2I position : path) {
          assert(network.railway.empty() || gf::manhattan_distance(network.railway.back(), position) == 1);
          network.railway.push_back(position);
        }
      }

      assert(gf::manhattan_distance(network.railway.back(), network.railway.front()) == 1);

      // determine if the train will ride clockwise or counterclockwise

      if (random->compute_bernoulli(0.5)) {
        std::reverse(network.railway.begin(), network.railway.end());
      }

      // determine the stop time

      const std::size_t total_travel_time = network.railway.size() * 5; // TODO: constant for train time
      const std::size_t total_stop_time = DayTime - total_travel_time;
      const std::size_t stop_time = total_stop_time / places.towns.size();
      const std::size_t remaining_stop_time = total_stop_time % places.towns.size();

      gf::Log::info("Train stop time: {} ({})", stop_time, stop_time + remaining_stop_time);

      for (const Town& town : places.towns) {
        const gf::Vec2I station = (town.rail_arrival + town.rail_departure) / 2;

        if (auto iterator = std::find(network.railway.begin(), network.railway.end(), station); iterator != network.railway.end()) {
          uint32_t index = uint32_t(std::distance(network.railway.begin(), iterator));

          if (network.stations.empty()) {
            assert(stop_time + remaining_stop_time <= std::numeric_limits<uint16_t>::max());
            network.stations.push_back({ index, uint16_t(stop_time + remaining_stop_time) });
          } else {
            assert(stop_time <= std::numeric_limits<uint16_t>::max());
            network.stations.push_back({ index, uint16_t(stop_time) });
          }

        } else {
          assert(false);
        }
      }

      // add the trains: at the beginning, one train arriving in each station

      for (const StationState& station : network.stations) {
        network.trains.push_back({ network.next_position(station.index) });
      }

      for (const gf::Vec2I position : network.railway) {
        state.cells(position).decoration = MapDecoration::Rail;

        for (const gf::Vec2I relative_neighbor : EightNeighbors) {
          state.cells(position + relative_neighbor).block = MapBlock::None;
        }

      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state, ImageType::Blocks);

        for (const Town& town : places.towns) {
          const gf::RectI town_space = gf::RectI::from_center_size(town.center, { 2 * TownRadius + 1, 2 * TownRadius + 1 });

          for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
            image.put_pixel(position, gf::Azure);
          }
        }

        for (const gf::Vec2I farm : places.farms) {
          const gf::RectI farm_space = gf::RectI::from_center_size(farm, { 2 * FarmRadius + 1, 2 * FarmRadius + 1 });

          for (const gf::Vec2I position : gf::rectangle_range(farm_space)) {
            image.put_pixel(position, gf::Azure);
          }
        }

        for (const gf::Vec2I position : network.railway) {
          image.put_pixel(position, gf::Black);
        }

        image.save_to_file("03_railways.png");
      }

      gf::Log::info("Railway length: {}", network.railway.size());

      return network;
    }


    /*
     * Step W. Create towns
     */



    /*
     * Step X. Compute the regions.
     *
     * Thanks to the biomes, contiguous regions are defined they will be used
     * in the following steps.
     */

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

      // compute region bounds

      auto compute_bounds = [](std::vector<WorldRegion>& regions, std::string_view name) {

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

        // for (WorldRegion& region : regions) {
        //   gf::Log::info("\t\t- Size: {}, Extent: {}x{}, Density: {:g}", region.points.size(), region.bounds.extent.w, region.bounds.extent.h, double(region.points.size()) / double(region.bounds.extent.w * region.bounds.extent.h));
        // }
      };

      compute_bounds(regions.prairie_regions, "Prairie");
      compute_bounds(regions.desert_regions, "Desert");
      compute_bounds(regions.forest_regions, "Forest");
      compute_bounds(regions.mountain_regions, "Moutain");

      return regions;
    }


    gf::Vec2I compute_starting_position(const MapState& map)
    {
      const gf::Vec2I center = WorldSize / 2;

      auto iterator = std::min_element(map.network.stations.begin(), map.network.stations.end(), [&](const StationState& lhs, const StationState& rhs) {
        const gf::Vec2I lhs_position = map.network.railway[lhs.index];
        const gf::Vec2I rhs_position = map.network.railway[rhs.index];
        return gf::manhattan_distance(center, lhs_position) < gf::manhattan_distance(center, rhs_position);
      });

      assert(iterator != map.network.stations.end());

      const gf::Vec2I position = map.network.railway[iterator->index];
      return position + gf::sign(position - center);
    }

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

    [[maybe_unused]] auto places = generate_places(state.map, random);
    gf::Log::info("- places ({:g}s)", clock.elapsed_time().as_seconds());

    state.map.network = generate_network(raw, state.map, places, random);
    gf::Log::info("- network ({:g}s)", clock.elapsed_time().as_seconds());

    [[maybe_unused]] auto regions = compute_regions(state.map);
    gf::Log::info("- regions ({:g}s)", clock.elapsed_time().as_seconds());


    // state.map = generate_map(outline, random);

    ActorState hero = {};
    hero.data = "Hero";
    hero.position = compute_starting_position(state.map);
    state.actors.push_back(hero);
    state.scheduler.queue.push({state.current_date, TaskType::Actor, 0});

    ActorState cow = {};
    cow.data = "Cow";
    cow.position = hero.position + gf::dirx(10);
    state.actors.push_back(cow);
    state.scheduler.queue.push({state.current_date + 1, TaskType::Actor, 1});

    for (const auto& [ index, train ] : gf::enumerate(state.map.network.trains)) {
      state.scheduler.queue.push({state.current_date, TaskType::Train, uint32_t(index) } );
    }

    const std::string name = random->compute_bernoulli(0.5) ? generate_random_female_name(random) : generate_random_male_name(random);
    gf::Log::info("Name: {}", name);

    state.log.messages.push_back({ state.current_date, fmt::format("Hello <style=character>{}</>!", name) });

    gf::Log::info("- actors ({:g}s)", clock.elapsed_time().as_seconds());

    return state;
  }

}
