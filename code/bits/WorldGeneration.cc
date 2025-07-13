#include "WorldGeneration.h"

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

#include "ActorState.h"
#include "Colors.h"
#include "Date.h"
#include "MapCell.h"
#include "MapState.h"
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

    constexpr int32_t ReducedFactor = 3;

    constexpr int32_t ReducedTownDiameter = TownDiameter / ReducedFactor;
    constexpr int32_t TownMinDistanceFromOther = 1500;

    constexpr std::size_t FarmsCount = TownsCount * 5;
    constexpr int32_t FarmRadius = 10;
    constexpr int32_t FarmDiameter = 2 * FarmRadius + 1;
    constexpr int32_t ReducedFarmDiameter = FarmDiameter / ReducedFactor;
    constexpr int32_t FarmMinDistanceFromOther = 200;

    constexpr std::size_t DayTime = 24 * 60 * 60;
    constexpr int32_t RailSpacing = 2;

    constexpr int CliffThreshold = 2;

    constexpr float SlopeFactor = 225.0f;

    constexpr std::size_t RegionMinimumSize = 400;

    constexpr gf::Vec2I EightNeighbors[] = {
      // clang-format off
      { -1, -1 }, {  0, -1 }, { +1, -1 },
      { -1,  0 },             { +1,  0 },
      { -1, +1 }, {  0, +1 }, { +1, +1 },
      // clang-format on
    };

    constexpr std::size_t SurfacePerCave = 250;
    constexpr int32_t CaveMinDistance = 10;

    bool is_on_side(gf::Vec2I position)
    {
      return position.x == 0 || position.x == WorldBasicSize - 1 || position.y == 0 || position.y == WorldBasicSize - 1;
    }

    constexpr gf::Vec2I to_map(gf::Vec2I position)
    {
      return ReducedFactor * position + (ReducedFactor / 2);
    }

    constexpr gf::Vec2I to_reduced(gf::Vec2I position)
    {
      return position / ReducedFactor;
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
            color = type == ImageType::Basic || !is_blocking(cell.decoration) ? DesertColor : gf::darker(gf::Green, 0.3f);
            break;
          case MapRegion::Forest:
            color = type == ImageType::Basic || !is_blocking(cell.decoration) ? ForestColor : gf::darker(gf::Green, 0.7f);
            break;
          case MapRegion::Moutain:
            color = type == ImageType::Basic || !is_blocking(cell.decoration) ? MountainColor : gf::darker(MountainColor, 0.5f);;
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
              cell.decoration = MapDecoration::Cactus;
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
              cell.decoration = MapDecoration::Tree;
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
          state.cells(position).decoration = MapDecoration::Cliff;
        } else if (state.cells(position).region == MapRegion::Moutain && is_on_side(position)) {
          state.cells(position).decoration = MapDecoration::Cliff;
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

    struct OuterTown {
      gf::Vec2I center;
      gf::Vec2I rail_arrival;
      gf::Vec2I rail_departure;
    };

    struct WorldPlaces {
      std::array<OuterTown, TownsCount> towns;
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
      constexpr gf::RectI reduced_world_rectangle = gf::RectI::from_size(WorldSize / ReducedFactor);

      WorldPlaces places = {};

      // first generate towns

      int town_rounds = 0;

      for (;;) {
        [[maybe_unused]] int tries = 0;

        for (OuterTown& town : places.towns) {
          do {
            town.center = random->compute_position(reduced_world_rectangle);
            ++tries;
          } while (!can_have_place(state, to_map(town.center), TownRadius + RailSpacing * ReducedFactor));
        }

        const int32_t min_distance = places.min_distance_between_towns();
        // gf::Log::info("Found potential towns after {} tries with min distance {}", tries, min_distance);

        ++town_rounds;

        if (min_distance * ReducedFactor > TownMinDistanceFromOther) {
          break;
        }
      }

      gf::Log::info("Towns generated after {} rounds", town_rounds);

      // compute rail arrival/departure

      for (OuterTown& town : places.towns) {
        const gf::RectI town_space = gf::RectI::from_center_size(town.center, { ReducedTownDiameter, ReducedTownDiameter });
        const gf::Direction direction = gf::direction(gf::angle<float>(WorldCenter - to_map(town.center)));

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
            farm_position = random->compute_position(reduced_world_rectangle);
            ++tries;
          } while (!can_have_place(state, to_map(farm_position), FarmRadius));
        }

        const int32_t min_distance = places.min_distance_between_towns_and_farms();
        // gf::Log::info("Found potential farms after {} tries with min distance {}", tries, min_distance);

        ++farm_rounds;

        if (min_distance * ReducedFactor > FarmMinDistanceFromOther) {
          break;
        }
      }

      gf::Log::info("Farms generated after {} rounds", farm_rounds);

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state, ImageType::Blocks);

        for (const OuterTown& town : places.towns) {
          const gf::RectI town_space = gf::RectI::from_center_size(to_map(town.center), { TownDiameter, TownDiameter });

          for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
            image.put_pixel(position, gf::Azure);
          }
        }

        for (const gf::Vec2I farm : places.farms) {
          const gf::RectI farm_space = gf::RectI::from_center_size(to_map(farm), { FarmDiameter, FarmDiameter });

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

      gf::GridMap grid = gf::GridMap::make_orthogonal(WorldSize / ReducedFactor);

      for (const gf::Vec2I position : grid.position_range()) {
        const gf::Vec2I map_position = to_map(position);

        int cliffs = 0;

        for (const gf::Vec2I neighbor : state.cells.compute_24_neighbors_range(map_position)) {
          if (state.cells(neighbor).decoration == MapDecoration::Cliff) {
            ++cliffs;
          }
        }

        grid.set_walkable(position, cliffs <= CliffThreshold);
      }

      if constexpr (Debug) {
        gf::Image image(grid.size());

        for (const gf::Vec2I position : image.position_range()) {
          if (grid.walkable(position)) {
            image.put_pixel(position, gf::White);
          } else {
            image.put_pixel(position, gf::Black);
          }
        }

        image.save_to_file("03_railways_alt.png");
      }

      for (const OuterTown& town : places.towns) {
        const gf::RectI town_space = gf::RectI::from_center_size(town.center, { ReducedTownDiameter, ReducedTownDiameter });

        for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
          grid.set_walkable(position, false);
        }

        const gf::Vec2I step = gf::sign(town.rail_departure - town.rail_arrival);

        for (gf::Vec2I position = town.rail_arrival + step; position != town.rail_departure; position += step) {
          grid.set_walkable(position, false);
        }
      }

      for (const gf::Vec2I farm : places.farms) {
        const gf::RectI farm_space = gf::RectI::from_center_size(farm, { ReducedFarmDiameter, ReducedFarmDiameter }).grow_by(1);

        for (const gf::Vec2I position : gf::rectangle_range(farm_space)) {
          grid.set_walkable(to_reduced(position), false);
        }
      }

      std::vector<std::vector<gf::Vec2I>> paths;

      std::vector<std::size_t> ordered_towns(places.towns.size());
      std::iota(ordered_towns.begin(), ordered_towns.end(), 0);
      std::sort(ordered_towns.begin(), ordered_towns.end(), [&](std::size_t lhs, std::size_t rhs) {
        return gf::angle<float>(places.towns[lhs].center - to_reduced(WorldCenter)) < gf::angle<float>(places.towns[rhs].center - to_reduced(WorldCenter));
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
          const float slope = static_cast<float>(std::abs(raw(to_map(position)).altitude - raw(to_map(neighbor)).altitude)) / distance;
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

      // construct the whole path

      std::vector<gf::Vec2I> reduced_railway;

      for (const std::vector<gf::Vec2I>& path : paths) {
        for (const gf::Vec2I position : path) {
          assert(reduced_railway.empty() || gf::manhattan_distance(reduced_railway.back(), position) == 1);
          reduced_railway.push_back(position);
        }
      }

      assert(gf::manhattan_distance(reduced_railway.back(), reduced_railway.front()) == 1);

      // determine if the train will ride clockwise or counterclockwise

      if (random->compute_bernoulli(0.5)) {
        std::reverse(reduced_railway.begin(), reduced_railway.end());
      }

      // put back in map

      NetworkState network = {};

      for (const gf::Vec2I position : reduced_railway) {
        network.railway.push_back(to_map(position));
      }

      // determine the stop time

      const std::size_t total_travel_time = network.railway.size() * ReducedFactor * 5; // TODO: constant for train time
      const std::size_t total_stop_time = DayTime - total_travel_time;
      const std::size_t stop_time = total_stop_time / places.towns.size();
      const std::size_t remaining_stop_time = total_stop_time % places.towns.size();

      gf::Log::info("Train stop time: {} ({})", stop_time, stop_time + remaining_stop_time);

      for (const OuterTown& town : places.towns) {
        const gf::Vec2I station = to_map((town.rail_departure + town.rail_arrival) / 2);

        if (auto iterator = std::find(network.railway.begin(), network.railway.end(), station); iterator != network.railway.end()) {
          uint32_t index = ReducedFactor * uint32_t(std::distance(network.railway.begin(), iterator));

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
        network.trains.push_back({ station.index });
      }

      for (const gf::Vec2I position : network.railway) {
        for (const gf::Vec2I relative_neighbor : EightNeighbors) {
          state.cells(position + relative_neighbor).decoration = MapDecoration::None;
        }
      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state, ImageType::Blocks);

        for (const OuterTown& town : places.towns) {
          const gf::RectI town_space = gf::RectI::from_center_size(to_map(town.center), { TownDiameter, TownDiameter });

          for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
            image.put_pixel(position, gf::Azure);
          }
        }

        for (const gf::Vec2I farm : places.farms) {
          const gf::RectI farm_space = gf::RectI::from_center_size(to_map(farm), { FarmDiameter, FarmDiameter });

          for (const gf::Vec2I position : gf::rectangle_range(farm_space)) {
            image.put_pixel(position, gf::Azure);
          }
        }


        for (const gf::Vec2I position : network.railway) {
          image.put_pixel(position, gf::Black);
        }

        image.save_to_file("03_railways.png");
      }

      gf::Log::info("Railway length: {}", network.railway.size() * ReducedFactor);

      return network;
    }


    /*
     * Step W. Create towns
     */

    void generate_towns(MapState& map, const WorldPlaces& places, gf::Random* random)
    {
      std::array<Building, 20> buildings = {
        Building::Bank,
        Building::Casino,
        Building::Church,
        Building::ClothShop,
        Building::FoodShop,
        Building::Hotel,
        Building::House1,
        Building::House2,
        Building::House3,
        Building::MarshalOffice,
        Building::Restaurant,
        Building::Saloon,
        Building::School,
        Building::WeaponShop,

        Building::None,
        Building::None,
        Building::None,
        Building::None,
        Building::None,
        Building::None,
      };

      std::size_t building_index = 0;

      auto generate_building = [&](Building& building) {
        if (building == Building::Empty) {
          assert(building_index < buildings.size());
          building = buildings[building_index++];
        }
      };

      for (auto [ index, town ] : gf::enumerate(map.towns)) {
        std::shuffle(buildings.begin(), buildings.end(), random->engine());

        town.position = to_map(places.towns[index].center) - TownRadius;

        town.horizontal_street = random->compute_uniform_integer<uint8_t>(2, 5);
        town.vertical_street = random->compute_uniform_integer<uint8_t>(2, 5);

        const int up_building = town.horizontal_street - 1;
        const int down_building = town.horizontal_street;

        const int left_building = town.vertical_street - 1;
        const int right_building = town.vertical_street;

        building_index = 0;

        for (int i = 0; i < TownsBlockSize; ++i) {
          generate_building(town({ i, up_building }));
          generate_building(town({ i, down_building }));
        }

        for (int j = 0; j < TownsBlockSize; ++j) {
          generate_building(town({ left_building, j }));
          generate_building(town({ right_building, j }));
        }

        assert(building_index == buildings.size());
      }

      // move buildings near the crossing

      auto stack_buildings = [&](TownState& town, gf::Vec2I position, gf::Direction direction)
      {
        constexpr gf::RectI blocks = gf::RectI::from_size({ TownsBlockSize, TownsBlockSize });
        const gf::Vec2I step = gf::displacement(direction);

        gf::Vec2I current = position;

        while (blocks.contains(current)) {
          assert(town(current) != Building::Empty);

          if (town(current) != Building::None) {
            std::swap(town(current), town(position));
            position += step;
          }

          current += step;
        }
      };

      for (TownState& town : map.towns) {
        const int up_building = town.horizontal_street - 1;
        const int down_building = town.horizontal_street;

        const int left_building = town.vertical_street - 1;
        const int right_building = town.vertical_street;

        stack_buildings(town, { left_building, up_building }, gf::Direction::Left);
        stack_buildings(town, { left_building, up_building }, gf::Direction::Up);
        stack_buildings(town, { left_building, down_building }, gf::Direction::Left);
        stack_buildings(town, { left_building, down_building }, gf::Direction::Down);
        stack_buildings(town, { right_building, up_building }, gf::Direction::Right);
        stack_buildings(town, { right_building, up_building }, gf::Direction::Up);
        stack_buildings(town, { right_building, down_building }, gf::Direction::Right);
        stack_buildings(town, { right_building, down_building }, gf::Direction::Down);
      }


      // remove decorations from towns

      for (const TownState& town : map.towns) {
        const gf::RectI town_space = gf::RectI::from_position_size(town.position, { TownDiameter, TownDiameter });

        for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
          map.cells(position).decoration = MapDecoration::None;
        }
      }
    }

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

    /*
     * Step ... Underground
     */

    struct CaveAccess {
      gf::Vec2I entrance;
      gf::Vec2I exit;
    };

    CaveAccess compute_underground_cave_access(MapState& state, const WorldRegion& region, gf::Random* random)
    {
      for (;;) {
        const std::size_t index = random->compute_uniform_integer(region.points.size());
        const gf::Vec2I entrance = region.points[index];

        if (state.cells(entrance).decoration != MapDecoration::Cliff) {
          continue;
        }

        for (const gf::Vec2I exit : state.cells.compute_4_neighbors_range(entrance)) {
          if (state.cells(exit).decoration != MapDecoration::Cliff) {
            return { entrance, exit };
          }
        }
      }

      return {{ 0, 0 }, { 0, 0 }};
    }

    std::vector<CaveAccess> compute_underground_cave_accesses(MapState& state, const WorldRegion& region, gf::Random* random)
    {
      const std::size_t access_count = region.points.size() / SurfacePerCave;
      std::vector<CaveAccess> accesses(access_count);

      for (;;) {

        for (std::size_t i = 0; i < access_count; ++i) {
          accesses[i] = (compute_underground_cave_access(state, region, random));
        }

        auto min_distance = [&]() {
          int32_t min_distance = std::numeric_limits<int32_t>::max();

          for (std::size_t i = 0; i < access_count; ++i) {
            for (std::size_t j = i + 1; j < access_count; ++j) {
              const int32_t distance = gf::manhattan_distance(accesses[i].entrance, accesses[j].entrance);
              min_distance = std::min(min_distance, distance);
            }
          }

          return min_distance;
        };

        if (min_distance() >= CaveMinDistance) {
          return accesses;
        }
      }

      return {};
    }


    // bool compute_underground_cave(MapState& state, const WorldRegion& region, gf::Random* random)
    // {
    //   const auto [ access, exit  ] = compute_underground_cave_entrance(state, region, random);
    //
    //   return true;
    // }

    void compute_underground(MapState& state, const WorldRegions& regions, gf::Random* random)
    {
      state.underground = { WorldSize };

      for (const WorldRegion& region : regions.mountain_regions) {
        const std::vector<CaveAccess> accesses = compute_underground_cave_accesses(state, region, random);

        for (const auto [ entrance, exit ] : accesses) {
          state.underground(entrance).decoration = MapDecoration::None;
          state.underground(exit).decoration = MapDecoration::FloorUp;

          for (const gf::Vec2I cave : state.underground.compute_8_neighbors_range(entrance)) {
            state.underground(cave).decoration = MapDecoration::None;
          }

          state.cells(entrance).decoration = MapDecoration::FloorDown;
        }
      }
    }

    gf::Vec2I compute_starting_position(const NetworkState& network)
    {
      const gf::Vec2I center = WorldSize / 2;

      auto iterator = std::min_element(network.stations.begin(), network.stations.end(), [&](const StationState& lhs, const StationState& rhs) {
        const gf::Vec2I lhs_position = network.railway[lhs.index / ReducedFactor];
        const gf::Vec2I rhs_position = network.railway[rhs.index / ReducedFactor];
        return gf::manhattan_distance(center, lhs_position) < gf::manhattan_distance(center, rhs_position);
      });

      assert(iterator != network.stations.end());

      const gf::Vec2I position = network.railway[iterator->index / ReducedFactor];
      return position + 2 * gf::sign(position - center);
    }

    Gender generate_gender(gf::Random* random)
    {
      std::discrete_distribution distribution({ 50.0, 48.0, 2.0 });
      const uint8_t index = static_cast<uint8_t>(distribution(random->engine()));
      return static_cast<Gender>(index);
    }

    int8_t generate_attribute(gf::Random* random)
    {
      // 3d6 + 1

      int8_t attribute = 2;

      for (int i = 0; i < 3; ++i) {
        attribute += static_cast<int8_t>(1 + random->compute_uniform_integer(6));
      }

      return attribute;
    }

  }

  WorldState generate_world(gf::Random* random)
  {
    gf::Clock clock;

    WorldState state = {};
    state.current_date = Date::generate_random(random);
    gf::Log::info("Starting generation...");
    const RawWorld raw = generate_raw(random);
    gf::Log::info("- raw ({:g}s)", clock.elapsed_time().as_seconds());
    state.map = generate_outline(raw, random);
    gf::Log::info("- outline ({:g}s)", clock.elapsed_time().as_seconds());
    generate_mountains(state.map, random);
    gf::Log::info("- moutains ({:g}s)", clock.elapsed_time().as_seconds());

    const WorldPlaces places = generate_places(state.map, random);
    gf::Log::info("- places ({:g}s)", clock.elapsed_time().as_seconds());

    state.network = generate_network(raw, state.map, places, random);
    gf::Log::info("- network ({:g}s)", clock.elapsed_time().as_seconds());

    generate_towns(state.map, places, random);
    gf::Log::info("- towns ({:g}s)", clock.elapsed_time().as_seconds());

    const WorldRegions regions = compute_regions(state.map);
    gf::Log::info("- regions ({:g}s)", clock.elapsed_time().as_seconds());

    compute_underground(state.map, regions, random);
    gf::Log::info("- underground ({:g}s)", clock.elapsed_time().as_seconds());

    // state.map = generate_map(outline, random);

    ActorState hero = {};
    hero.data = "Hero";
    hero.position = compute_starting_position(state.network);

    HumanFeature human;
    human.gender = generate_gender(random);

    switch (human.gender) {
      case Gender::Girl:
        human.name = generate_random_white_female_name(random);
        break;
      case Gender::Boy:
        human.name = generate_random_white_male_name(random);
        break;
      case Gender::NonBinary:
        human.name = generate_random_white_non_binary_name(random);
        break;
    }

    human.age = random->compute_uniform_integer<int8_t>(20, 40);
    human.birthday = generate_random_birthday(random);

    human.health = MaxHealth - 1;

    human.force = generate_attribute(random);
    human.dexterity = generate_attribute(random);
    human.constitution = generate_attribute(random);
    human.luck = generate_attribute(random);

    human.intensity = 100;
    human.precision = 90;
    human.endurance = 70;

    hero.feature = human;

    hero.weapon.data = "Colt Dragoon Revolver";
    hero.weapon.cartridges = 0;

    hero.ammunition.data = ".44 Ammunitions";
    hero.ammunition.count = 32;

    gf::Log::info("Name: {} (Luck: {})", human.name, human.luck);

    state.actors.push_back(hero);
    state.scheduler.queue.push({state.current_date, TaskType::Actor, 0});

    {
      ActorState cow = {};
      cow.data = "Cow";
      cow.position = hero.position + gf::dirx(10);

      AnimalFeature feature;
      feature.mounted_by = NoIndex;
      cow.feature = feature;

      state.actors.push_back(cow);

      Date cow_next_turn = state.current_date;
      cow_next_turn.add_seconds(1);
      state.scheduler.queue.push({cow_next_turn, TaskType::Actor, 1});
    }

    for (const auto& [ index, train ] : gf::enumerate(state.network.trains)) {
      Date date = state.current_date;
      date.add_seconds(state.network.stations[index].stop_time);
      state.scheduler.queue.push({ date, TaskType::Train, uint32_t(index) } );
    }

    state.add_message(fmt::format("Hello <style=character>{}</>!", human.name));

    gf::Log::info("- actors ({:g}s)", clock.elapsed_time().as_seconds());

    return state;
  }

}
