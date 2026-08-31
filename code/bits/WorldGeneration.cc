#include "WorldGeneration.h"

#include <cstdint>

#include <algorithm>
#include <queue>
#include <string_view>

#include <gf2/core/Array2D.h>
#include <gf2/core/Clock.h>
#include <gf2/core/Direction.h>
#include <gf2/core/Dice.h>
#include <gf2/core/Easing.h>
#include <gf2/core/FieldOfVision.h>
#include <gf2/core/Geometry.h>
#include <gf2/core/GridMap.h>
#include <gf2/core/Heightmap.h>
#include <gf2/core/Log.h>
#include <gf2/core/Noises.h>
#include <gf2/core/ProcGen.h>
#include <gf2/core/Vec2.h>

#include "ActorData.h"
#include "ActorGeneration.h"
#include "ActorState.h"
#include "Colors.h"
#include "Date.h"
#include "ItemState.h"
#include "MapBuilding.h"
#include "MapCell.h"
#include "MapCellBiome.h"
#include "MapState.h"
#include "Settings.h"
#include "Times.h"
#include "WorldData.h"

namespace fw {

  namespace {

    constexpr bool Debug = true;

    constexpr double WorldNoiseScale = WorldBasicSize / 256.0;

    constexpr int32_t WorldPaddingSize = 150;

    constexpr double RiverMinMoisture = 0.2;
    constexpr double RiverSourceMinMoisture = 0.7;
    constexpr int RiverMouthMinDistance = 3000;
    constexpr int RiverSourceMinDistance = 1500;
    constexpr float RiverSlopeFactor = 10000.0f;
    constexpr int RiverRadiusMin = 12;
    constexpr int RiverRadiusMax = RiverRadiusMin + 10;
    constexpr std::size_t RiverPart = 800;

    constexpr double AltitudeThreshold = 0.55;
    constexpr double MoistureLoThreshold = 0.45;
    constexpr double MoistureHiThreshold = 0.55;

    constexpr double PrairieHerbProbability = 0.2;
    constexpr double DesertCactusProbability = 0.02;
    constexpr double ForestTreeProbability = 0.25;
    constexpr double WaterWaveProbability = 0.2;

    constexpr float MoutainThreshold       = 0.4f;
    constexpr int MoutainSurvivalThreshold = 6;
    constexpr int MoutainBirthThreshold    = 8;
    constexpr int MoutainIterations        = 7;


    constexpr int32_t ReducedFactor = 3;

    constexpr int32_t ReducedTownDiameter = TownDiameter / ReducedFactor;
    constexpr int32_t TownMinDistanceFromOther = 1500;

    constexpr int32_t ReducedLocalityDiameter = LocalityDiameter / ReducedFactor;
    constexpr int32_t LocalityMinDistanceFromOther = 250;

    constexpr int32_t RailSpacing = 2;

    constexpr int CliffThreshold = 2;

    constexpr float SlopeFactor = 225.0f;
    constexpr float RiverPenalty = 10.0f;
    constexpr float RailBlockPenalty = 1.5f;
    constexpr float DoubleRailBlockPenalty = 5.0f;

    constexpr int32_t RoadMaxDistanceFromLocality = 300;
    constexpr int32_t RoadMaxDistanceFromTown = 350;
    static_assert(LocalityMinDistanceFromOther < RoadMaxDistanceFromLocality && RoadMaxDistanceFromTown < TownMinDistanceFromOther);

    constexpr std::size_t RegionMinimumSize = 400;

    constexpr gf::Vec2I NineArea[] = {
      // clang-format off
      { -1, -1 }, {  0, -1 }, { +1, -1 },
      { -1,  0 }, {  0,  0 }, { +1,  0 },
      { -1, +1 }, {  0, +1 }, { +1, +1 },
      // clang-format on
    };

    constexpr std::size_t SurfacePerCave = 2000;
    constexpr std::size_t MaxCaveAccessTries = 20;
    constexpr int32_t CaveMinDistance = 10;
    constexpr int32_t CaveLinkDistance = 70;

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

    gf::Image compute_basic_image(const BackgroundMap& state, ImageType type = ImageType::Basic) {
      gf::Image image(WorldSize);

      for (const gf::Vec2I position : image.position_range()) {
        const MapCell& cell = state(position);
        gf::Color color = gf::Transparent;

        switch (cell.region) {
          case MapCellBiome::None:
            color = gf::Transparent;
            break;
          case MapCellBiome::Prairie:
            color = PrairieColor;
            break;
          case MapCellBiome::Desert:
            color = DesertColor;
            break;
          case MapCellBiome::Forest:
            color = ForestColor;
            break;
          case MapCellBiome::Mountain:
            color = MountainColor;
            break;
          case MapCellBiome::Underground:
            color = DirtColor;
            break;
          case MapCellBiome::Building:
            color = StreetColor;
            break;
        }

        if (type != ImageType::Basic) {
          switch (cell.decoration) {
            case MapCellDecoration::None:
            case MapCellDecoration::FloorDown:
            case MapCellDecoration::FloorUp:
            case MapCellDecoration::Herb:
            case MapCellDecoration::Wall:
              // nothing
              break;
            case MapCellDecoration::Cactus:
              color = gf::darker(gf::Green, 0.3f);
              break;
            case MapCellDecoration::Tree:
              color = gf::darker(gf::Green, 0.7f);
              break;
            case MapCellDecoration::Water:
              color = gf::Azure;
              break;
            case MapCellDecoration::Cliff:
              color = gf::darker(MountainColor, 0.5f);
              break;
            case MapCellDecoration::Rock:
              color = RockColor;
              break;
          }
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

    float distance_for_rivers(const RawWorld& raw, gf::Vec2I position, gf::Vec2I neighbor)
    {
      neighbor = gf::clamp(neighbor, { 0, 0 }, raw.size() - 1);

      if (position == neighbor) {
        return 1e10;
      }

      const float distance = gf::euclidean_distance<float>(position, neighbor);
      const float slope = static_cast<float>(std::abs(raw(position).altitude - raw(neighbor).altitude)) / distance;

      const float value = distance * (1 + RiverSlopeFactor * gf::square(slope));

      const double moisture = raw(neighbor).moisture;

      if (moisture < MoistureLoThreshold) {
        return value * 10;
      }

      if (moisture < MoistureHiThreshold) {
        return value * 5;
      }

      return value;
    }

    struct River {
      std::vector<gf::Vec2I> path;
    };

    std::vector<River> generate_rivers(const RawWorld& raw, gf::Random* random)
    {
      std::vector<River> rivers;

      auto compute_random_source = [&]() {
        constexpr gf::RectI area = gf::RectI::from_size(WorldSize).shrink_by(WorldBasicSize / 4);

        for (;;) {
          const gf::Vec2I source = random->compute_position(area);
          const double moisture = raw(source).moisture;

          if (moisture > RiverSourceMinMoisture) {
            return source;
          }
        }
      };

      auto compute_random_out = [&]() {
        constexpr gf::RectI area = gf::RectI::from_size(WorldSize).shrink_by(WorldBasicSize / 8);
        gf::Vec2I out = random->compute_position(area);

        if (random->compute_bernoulli(0.5)) {
          // push it horizontally
          if (random->compute_bernoulli(0.5)) {
            out.x = 0;
          } else {
            out.x = WorldBasicSize - 1;
          }
        } else {
          // push it vertically
          if (random->compute_bernoulli(0.5)) {
            out.y = 0;
          } else {
            out.y = WorldBasicSize - 1;
          }
        }

        return out;
      };

      // compute river points

      std::array<gf::Vec2I, 3> points;
      std::size_t round = 0;

      for (;;) {
        points[0] = compute_random_out();
        points[1] = compute_random_source();
        points[2] = compute_random_source();

        ++round;

        if (gf::square_distance(points[0], points[1]) < gf::square(RiverMouthMinDistance)) {
          continue;
        }

        if (gf::square_distance(points[0], points[2]) < gf::square(RiverMouthMinDistance)) {
          continue;
        }

        if (gf::square_distance(points[1], points[2]) < gf::square(RiverSourceMinDistance)) {
          continue;
        }

        break;
      }

      gf::Log::debug("\tFound river points after {} rounds", round);

      // compute the rivers

      gf::GridMap grid = gf::GridMap::make_orthogonal(WorldSize);

      for (const gf::Vec2I position : raw.position_range()) {
        if (raw(position).moisture < RiverMinMoisture) {
          grid.set_walkable(position, false);
        }
      }

      for (const gf::Vec2I source : { points[1], points[2] }) {
        std::vector<gf::Vec2I> river = grid.compute_route(source, points[0], [&](gf::Vec2I position, gf::Vec2I neighbor) {
          return distance_for_rivers(raw, position, neighbor);
        });

        gf::Log::debug("\triver: length = {}", river.size());

        rivers.emplace_back(std::move(river));
      }

      return rivers;
    }

    RawWorld modify_raw_with_rivers(const RawWorld& raw, const std::vector<River>& rivers, gf::Random* random)
    {
      RawWorld new_raw(raw);

      for (const River& river : rivers) {
        const int radius = random->compute_uniform_integer(RiverRadiusMin, RiverRadiusMax);
        const int radius_square = gf::square(radius);

        for (const gf::Vec2I position : river.path) {
          // elevate humidity around the river

          new_raw(position).moisture = 1.0; // std::max(new_raw(position).moisture, std::min(raw(position).moisture * 1.5, 1.0));
          const double height = new_raw(position).moisture;
          const double factor = height / radius_square;


          for (const gf::Vec2I neighbor : gf::neighbor_square_range(position, radius)) {
            if (!raw.valid(neighbor)) {
              continue;
            }

            const int distance_square = gf::square_distance(position, neighbor);

            if (distance_square < radius_square) {
              double moisture_offset = factor * (radius_square - distance_square);
              new_raw(neighbor).moisture = std::max(new_raw(neighbor).moisture, raw(neighbor).moisture + moisture_offset);
            }
          }
        }
      }

      return new_raw;
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

    MapState generate_outline(const RawWorld& raw, gf::Random* random, const std::vector<River>& rivers)
    {
      MapState state = {};
      state.ground = { WorldSize };

      for (const gf::Vec2I position : state.ground.position_range()) {
        MapCell& cell = state.ground(position);
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
            cell.region = MapCellBiome::Desert;

            if (random->compute_bernoulli(DesertCactusProbability * raw_cell.moisture / MoistureLoThreshold)) {
              cell.decoration = MapCellDecoration::Cactus;
            }
          } else {
            cell.region = MapCellBiome::Prairie;

            if (random->compute_bernoulli(PrairieHerbProbability * raw_cell.moisture)) {
              cell.decoration = MapCellDecoration::Herb;
            }
          }
        } else {
          if (raw_cell.moisture < MoistureHiThreshold) {
            cell.region = MapCellBiome::Mountain;

            // cliffs are put later
          } else {
            cell.region = MapCellBiome::Forest;

            if (is_on_side(position) || random->compute_bernoulli(ForestTreeProbability * raw_cell.moisture)) {
              cell.decoration = MapCellDecoration::Tree;
            }
          }
        }
      }

      for (const River& river : rivers) {
        for (const auto [ index, position ] : gf::enumerate(river.path)) {
          state.ground(position).decoration = MapCellDecoration::Water;

          const int width = 1 + static_cast<int>(index / RiverPart);

          for (const gf::Vec2I neighbor : gf::neighbor_diamond_range(position, width)) {
            if (!state.ground.valid(neighbor)) {
              continue;
            }

            MapCell& cell = state.ground(neighbor);
            cell.decoration = MapCellDecoration::Water;
          }
        }
      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state.ground);
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

      for (const gf::Vec2I position : state.ground.position_range()) {
        if (state.ground(position).region == MapCellBiome::Mountain) {
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
          if (state.ground(position).region != MapCellBiome::Mountain) {
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
        if (state.ground(position).region != MapCellBiome::Mountain) {
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
          state.ground(position).decoration = MapCellDecoration::Cliff;
        } else if (state.ground(position).region == MapCellBiome::Mountain && is_on_side(position)) {
          state.ground(position).decoration = MapCellDecoration::Cliff;
        }
      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state.ground, ImageType::Blocks);
        image.save_to_file("01_blocks.png");
      }

    }

    /*
     * Step 3. Generate towns and localities.
     *
     */

    struct OuterTown {
      gf::Vec2I center;
      gf::Vec2I rail_arrival;
      gf::Vec2I rail_departure;
      gf::Vec2I road_from_prev;
      gf::Vec2I road_to_next;
    };

    struct OuterLocality {
      gf::Vec2I center;
      LocalityType type = LocalityType::Farm;
      int32_t distance = 0;
    };

    struct WorldPlaces {
      std::array<OuterTown, TownsCount> towns;
      std::array<OuterLocality, LocalityCount> localities;

      int32_t min_distance_between_towns() const
      {
        gf::Span<const OuterTown> other_towns(towns.data(), towns.size());

        int32_t min_distance = std::numeric_limits<int32_t>::max();

        for (const OuterTown& town : towns) {
          other_towns = other_towns.last_except<1>();
          const int32_t distance = min_distance_to(town.center, other_towns);
          min_distance = std::min(min_distance, distance);
        }

        return min_distance;
      }

      int32_t min_distance_between_towns_and_localities() const
      {
        gf::Span<const OuterTown> other_towns(towns.data(), towns.size());
        gf::Span<const OuterLocality> other_localities(localities.data(), localities.size());

        int32_t min_distance = std::numeric_limits<int32_t>::max();

        for (const OuterLocality& locality : localities) {
          other_localities = other_localities.last_except<1>();
          const int32_t distance_to_localities = min_distance_to(locality.center, other_localities);
          const int32_t distance_to_towns = min_distance_to(locality.center, other_towns);
          min_distance = std::min({ min_distance, distance_to_localities, distance_to_towns });
        }

        return min_distance;
      }

      template<typename T>
      int32_t min_distance_to(gf::Vec2I origin, gf::Span<const T> others) const
      {
        int32_t min_distance = std::numeric_limits<int32_t>::max();

        for (const T& other : others) {
          int32_t distance = gf::manhattan_distance(origin, other.center);

          if (distance > 0) {
            min_distance = std::min(min_distance, distance);
          }
        }

        return min_distance;
      }

    };

    gf::Image compute_image_add_towns_and_localities(const gf::Image& original, const WorldPlaces& places)
    {
      gf::Image image(original);

      for (const OuterTown& town : places.towns) {
        const gf::RectI town_space = gf::RectI::from_center_size(to_map(town.center), { TownDiameter, TownDiameter });

        for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
          image.put_pixel(position, gf::Purple);
        }
      }

      for (const OuterLocality& locality : places.localities) {
        const gf::RectI locality_space = gf::RectI::from_center_size(to_map(locality.center), { LocalityDiameter, LocalityDiameter });

        gf::Color color = gf::Black;

        switch (locality.type) {
          case LocalityType::Farm:
            color = gf::Chartreuse;
            break;
          case LocalityType::Camp:
            color = gf::Cerulean;
            break;
          case LocalityType::Village:
            color = gf::Vermilion;
            break;
        }

        for (const gf::Vec2I position : gf::rectangle_range(locality_space)) {
          image.put_pixel(position, color);
        }
      }

      return image;
    }


    bool can_have_place(const MapState& state, gf::Vec2I position, int32_t radius)
    {
      assert(state.ground.valid(position));

      if (state.ground(position).region != MapCellBiome::Prairie) {
        return false;
      }

      for (const gf::Vec2I neighbor : state.ground.square_range(position, radius)) {
        if (!state.ground.valid(neighbor)) {
          return false;
        }

        const MapCell& cell = state.ground(neighbor);

        if (cell.region != MapCellBiome::Prairie) {
          return false;
        }

        if (cell.decoration == MapCellDecoration::Water) {
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

        auto put_rail_at = [&](gf::Orientation orientation) {
          return town_space.position_at(orientation) + RailSpacing * gf::displacement(orientation);
        };

        auto put_road_at = [&](gf::Orientation orientation) {
          return town_space.position_at(orientation);
        };

        switch (direction) {
          case gf::Direction::Up:
            town.rail_arrival = put_rail_at(gf::Orientation::NorthEast);
            town.rail_departure = put_rail_at(gf::Orientation::NorthWest);
            town.road_from_prev = put_road_at(gf::Orientation::East);
            town.road_to_next = put_road_at(gf::Orientation::West);
            break;
          case gf::Direction::Right:
            town.rail_arrival = put_rail_at(gf::Orientation::SouthEast);
            town.rail_departure = put_rail_at(gf::Orientation::NorthEast);
            town.road_from_prev = put_road_at(gf::Orientation::South);
            town.road_to_next = put_road_at(gf::Orientation::North);
            break;
          case gf::Direction::Down:
            town.rail_arrival = put_rail_at(gf::Orientation::SouthWest);
            town.rail_departure = put_rail_at(gf::Orientation::SouthEast);
            town.road_from_prev = put_road_at(gf::Orientation::West);
            town.road_to_next = put_road_at(gf::Orientation::East);
            break;
          case gf::Direction::Left:
            town.rail_arrival = put_rail_at(gf::Orientation::NorthWest);
            town.rail_departure = put_rail_at(gf::Orientation::SouthWest);
            town.road_from_prev = put_road_at(gf::Orientation::North);
            town.road_to_next = put_road_at(gf::Orientation::South);
            break;
          case gf::Direction::Center:
            assert(false);
            break;
        }

      }

      // sort towns

      std::sort(places.towns.begin(), places.towns.end(), [&](const OuterTown& lhs, const OuterTown& rhs) {
        return gf::angle<float>(lhs.center - to_reduced(WorldCenter)) < gf::angle<float>(rhs.center - to_reduced(WorldCenter));
      });


      // second generate localities

      int locality_rounds = 0;

      for (;;) {
        [[maybe_unused]] int tries = 0;

        for (OuterLocality& locality : places.localities) {
          do {
            locality.center = random->compute_position(reduced_world_rectangle);
            ++tries;
          } while (!can_have_place(state, to_map(locality.center), LocalityRadius));
        }

        const int32_t min_distance = places.min_distance_between_towns_and_localities();
        // gf::Log::info("Found potential locality after {} tries with min distance {}", tries, min_distance);

        ++locality_rounds;

        if (min_distance * ReducedFactor > LocalityMinDistanceFromOther) {
          break;
        }
      }

      // determine villages

      gf::Span<const OuterTown> other_towns(places.towns.data(), places.towns.size());

      for (OuterLocality& locality : places.localities) {
        locality.distance = places.min_distance_to(locality.center, other_towns);
      }

      std::sort(places.localities.begin(), places.localities.end(), [](const OuterLocality& lhs, const OuterLocality& rhs) {
        return lhs.distance > rhs.distance;
      });

      for (std::size_t i = 0; i < TownsCount; ++i) {
        places.localities[i].type = LocalityType::Village;
      }

      // determine camps

      for (const OuterTown& town : places.towns) {
        auto iterator = std::min_element(places.localities.begin(), places.localities.end(), [&town](const OuterLocality& lhs, const OuterLocality& rhs) {
          return gf::manhattan_distance(town.center, lhs.center) < gf::manhattan_distance(town.center, rhs.center);
        });

        assert(iterator->type != LocalityType::Village);
        iterator->type = LocalityType::Camp;
      }

      gf::Log::info("Localities generated after {} rounds", locality_rounds);

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state.ground, ImageType::Blocks);
        image = compute_image_add_towns_and_localities(image, places);
        image.save_to_file("02_places.png");
      }

      return places;
    }

    /*
     * Step 4. Generate railway
     */

    gf::Image compute_image_add_network(const gf::Image& original, const NetworkState& network)
    {
      gf::Image image(original);

      for (const gf::Vec2I position : network.railway) {
        image.put_pixel(position, gf::Black);
      }

      for (const gf::Vec2I position : network.roads) {
        image.put_pixel(position, gf::Gray);
      }

      return image;
    }

    gf::GridMap compute_basic_grid(const MapState& state)
    {
      gf::GridMap grid = gf::GridMap::make_orthogonal(WorldSize / ReducedFactor);

      for (const gf::Vec2I position : grid.position_range()) {
        const gf::Vec2I map_position = to_map(position);

        int cliffs = 0;

        for (const gf::Vec2I neighbor : state.ground.compute_24_neighbors_range(map_position)) {
          if (state.ground(neighbor).decoration == MapCellDecoration::Cliff) {
            ++cliffs;
          }
        }

        grid.set_walkable(position, cliffs <= CliffThreshold);
      }

      return grid;
    }

    float distance_with_slope_reduced(const RawWorld& raw, gf::Vec2I position, gf::Vec2I neighbor)
    {
      const float distance = gf::euclidean_distance<float>(position, neighbor);
      const float slope = static_cast<float>(std::abs(raw(to_map(position)).altitude - raw(to_map(neighbor)).altitude)) / distance;
      return distance * (1 + SlopeFactor * gf::square(slope));
    }

    bool has_water_nearby(const MapState& state, gf::Vec2I position) {
      if (state.ground(position).decoration == MapCellDecoration::Water) {
        return true;
      }

      for (const gf::Vec2I neighbor : state.ground.compute_8_neighbors_range(position)) {
        if (state.ground(neighbor).decoration == MapCellDecoration::Water) {
          return true;
        }
      }

      return false;
    }

    NetworkState generate_network(const RawWorld& raw, MapState& state, const WorldPlaces& places, gf::Random* random)
    {
      // initialize the grid

      gf::GridMap grid = compute_basic_grid(state);

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

      for (const OuterLocality& locality : places.localities) {
        const gf::RectI locality_space = gf::RectI::from_center_size(locality.center, { ReducedLocalityDiameter, ReducedLocalityDiameter }).grow_by(1);

        for (const gf::Vec2I position : gf::rectangle_range(locality_space)) {
          grid.set_walkable(position, false);
        }
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

      std::vector<std::vector<gf::Vec2I>> paths;

      for (std::size_t i = 0; i < places.towns.size(); ++i) {
        // path for the station

        std::vector<gf::Vec2I> station;
        const gf::Vec2I step = gf::sign(places.towns[i].rail_departure - places.towns[i].rail_arrival);

        for (gf::Vec2I position = places.towns[i].rail_arrival + step; position != places.towns[i].rail_departure; position += step) {
          station.push_back(position);
        }

        paths.push_back(std::move(station));

        // path to the next town

        const std::size_t j = (i + 1) % places.towns.size();
        auto path = grid.compute_route(places.towns[i].rail_departure, places.towns[j].rail_arrival, [&](gf::Vec2I position, gf::Vec2I neighbor) {
          const float distance = distance_with_slope_reduced(raw, position, neighbor);

          if (has_water_nearby(state, to_map(neighbor))) {
            return distance * RiverPenalty;
          }

          return distance;
        });

        for (const gf::Vec2I point : path) {
          for (const gf::Vec2I relative_neighbor : NineArea) {
            grid.set_walkable(point + relative_neighbor, false);
          }
        }

        assert(!path.empty()); // TODO: what to do in this case? it happens from time to time
        gf::Log::info("Points between {} and {}: {}", i, j, path.size());

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

      const Second total_travel_time = static_cast<Second>(network.railway.size()) * ReducedFactor * TrainTime;
      const Second total_stop_time = DayTime - total_travel_time;
      const Second stop_time = total_stop_time / static_cast<Second>(places.towns.size());
      const Second remaining_stop_time = total_stop_time % static_cast<Second>(places.towns.size());

      gf::Log::info("Train stop time: {} ({})", stop_time, stop_time + remaining_stop_time);

      for (const OuterTown& town : places.towns) {
        const gf::Vec2I station = to_map((town.rail_departure + town.rail_arrival) / 2);

        if (auto iterator = std::find(network.railway.begin(), network.railway.end(), station); iterator != network.railway.end()) {
          uint32_t index = ReducedFactor * uint32_t(std::distance(network.railway.begin(), iterator));

          if (network.stations.empty()) {
            assert(stop_time + remaining_stop_time <= std::numeric_limits<uint16_t>::max());
            network.stations.push_back({ index, stop_time + remaining_stop_time });
          } else {
            assert(stop_time <= std::numeric_limits<uint16_t>::max());
            network.stations.push_back({ index, stop_time });
          }

        } else {
          assert(false);
        }
      }

      for (const gf::Vec2I position : network.railway) {
        for (const gf::Vec2I relative_neighbor : NineArea) {
          MapCell& cell = state.ground(position + relative_neighbor);

          if (cell.decoration != MapCellDecoration::Water) {
            cell.decoration = MapCellDecoration::None;
          }
        }
      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state.ground, ImageType::Blocks);
        image = compute_image_add_towns_and_localities(image, places);
        image = compute_image_add_network(image, network);
        image.save_to_file("03_railways.png");
      }

      gf::Log::info("Railway length: {}", network.railway.size() * ReducedFactor);

      return network;
    }

    /*
     * Step ?. Generate roads
     *
     *
     */

    void generate_roads(const RawWorld& raw, const MapState& state, NetworkState& network, const WorldPlaces& places)
    {
      gf::GridMap grid = compute_basic_grid(state);

      for (const OuterTown& town : places.towns) {
        const gf::RectI town_space = gf::RectI::from_center_size(town.center, { ReducedTownDiameter, ReducedTownDiameter });

        for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
          grid.set_blocked(position);
        }
      }

      for (const OuterLocality& locality : places.localities) {
        const gf::RectI locality_space = gf::RectI::from_center_size(locality.center, { ReducedLocalityDiameter, ReducedLocalityDiameter }).grow_by(1);

        for (const gf::Vec2I position : gf::rectangle_range(locality_space)) {
          grid.set_blocked(position);
        }
      }

      for (const gf::Vec2I map_position : network.railway) {
        const gf::Vec2I position = to_reduced(map_position);
        grid.set_blocked(position);
      }

      std::vector<gf::Vec2I> roads;

      const auto distance_function = [&](gf::Vec2I position, gf::Vec2I neighbor) {
        const float distance = distance_with_slope_reduced(raw, position, neighbor);

        if (has_water_nearby(state, to_map(neighbor))) {
          return RiverPenalty * distance;
        }

        if (grid.blocked(neighbor)) {
          if (grid.blocked(position)) {
            return DoubleRailBlockPenalty * distance;
          }

          return RailBlockPenalty * distance;
        }

        return distance;
      };

      const auto position_comparator = [](gf::Vec2I lhs, gf::Vec2I rhs) {
        return std::tie(lhs.x, lhs.y) < std::tie(rhs.x, rhs.y);
      };

      for (const OuterLocality& from : places.localities) {
        for (const OuterLocality& to : places.localities) {
          if (from.center == to.center) {
            continue;
          }

          if (position_comparator(from.center, to.center)) {
            continue; // to prevent double edges
          }

          if (gf::manhattan_distance(from.center, to.center) > RoadMaxDistanceFromLocality) {
            continue;
          }

          const std::vector<gf::Vec2I> road = grid.compute_route(from.center, to.center, distance_function);
          roads.insert(roads.end(), road.begin(), road.end());
        }
      }

      for (const OuterTown& from : places.towns) {
        for (const OuterLocality& to : places.localities) {
          if (from.center == to.center) {
            continue;
          }

          if (gf::manhattan_distance(from.center, to.center) > RoadMaxDistanceFromTown) {
            continue;
          }

          const std::vector<gf::Vec2I> road = grid.compute_route(from.center, to.center, distance_function);
          roads.insert(roads.end(), road.begin(), road.end());
        }
      }

      std::sort(roads.begin(), roads.end(), position_comparator);

      roads.erase(std::unique(roads.begin(), roads.end()), roads.end());

      roads.erase(std::remove_if(roads.begin(), roads.end(), [&](const gf::Vec2I road) {
        for (const OuterTown& town : places.towns) {
          const gf::RectI town_space = gf::RectI::from_center_size(town.center, { ReducedTownDiameter, ReducedTownDiameter });

          if (town_space.contains(road)) {
            return true;
          }
        }

        for (const OuterLocality& locality : places.localities) {
          const gf::RectI locality_space = gf::RectI::from_center_size(locality.center, { ReducedLocalityDiameter, ReducedLocalityDiameter });

          if (locality_space.contains(road)) {
            return true;
          }
        }

        return false;
      }), roads.end());

      for (const gf::Vec2I position : roads) {
        network.roads.push_back(to_map(position));
      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state.ground, ImageType::Blocks);
        image = compute_image_add_towns_and_localities(image, places);
        image = compute_image_add_network(image, network);
        image.save_to_file("04_roads.png");
      }
    }

    /*
     * Step W. Create towns
     */

    void generate_towns(MapState& map, const WorldPlaces& places, gf::Random* random)
    {
      std::array<BuildingType, 20> buildings = {
        BuildingType::Bank,
        BuildingType::Casino,
        BuildingType::Church,
        BuildingType::ClothShop,
        BuildingType::FoodShop,
        BuildingType::Hotel,
        BuildingType::House1,
        BuildingType::House2,
        BuildingType::House3,
        BuildingType::MarshalOffice,
        BuildingType::Restaurant,
        BuildingType::Saloon,
        BuildingType::School,
        BuildingType::WeaponShop,

        BuildingType::None,
        BuildingType::None,
        BuildingType::None,
        BuildingType::None,
        BuildingType::None,
        BuildingType::None,
      };

      std::size_t building_index = 0;

      auto generate_building = [&](BuildingType& building) {
        if (building == BuildingType::Empty) {
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
          generate_building(town({ i, up_building }).type);
          generate_building(town({ i, down_building }).type);
        }

        for (int j = 0; j < TownsBlockSize; ++j) {
          generate_building(town({ left_building, j }).type);
          generate_building(town({ right_building, j }).type);
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
          assert(town(current).type != BuildingType::Empty);

          if (town(current).type != BuildingType::None) {
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

      // compute building direction

      for (TownState& town : map.towns) {
        const int up_building = town.horizontal_street - 1;
        const int down_building = town.horizontal_street;

        const int left_building = town.vertical_street - 1;
        const int right_building = town.vertical_street;

        for (int32_t i = 0; i < TownsBlockSize; ++i) {
          for (int32_t j = 0; j < TownsBlockSize; ++j) {
            const gf::Vec2I block_position = { i, j };

            if (town(block_position).type == BuildingType::Empty || town(block_position).type == BuildingType::None) {
              continue;
            }

            gf::Direction direction = gf::Direction::Center;

            if (j == up_building) {
              direction = gf::Direction::Up;
            } else if (j == down_building) {
              direction = gf::Direction::Down;
            }

            if (i == left_building) {
              direction = gf::Direction::Left;
            } else if (i == right_building) {
              direction = gf::Direction::Right;
            }

            assert(direction != gf::Direction::Center);
            town(block_position).direction = direction;
          }
        }
      }

      // remove decorations from towns

      for (const TownState& town : map.towns) {
        const gf::RectI town_space = gf::RectI::from_position_size(town.position, { TownDiameter, TownDiameter });

        for (const gf::Vec2I position : gf::rectangle_range(town_space)) {
          map.ground(position).decoration = MapCellDecoration::None;
        }
      }

      // put walls

      for (const TownState& town : map.towns) {
        for (int32_t i = 0; i < TownsBlockSize; ++i) {
          for (int32_t j = 0; j < TownsBlockSize; ++j) {
            const gf::Vec2I block_position = { i, j };
            const Building& building = town(block_position);

            if (building.type == BuildingType::Empty || building.type == BuildingType::None) {
              continue;
            }

            const TownBuildingPlan& plan = compute_town_building_plan(building.type);

            for (int32_t y = 0; y < TownBuildingSize; ++y) {
              for (int32_t x = 0; x < TownBuildingSize; ++x) {
                const gf::Vec2I position = { x, y };
                const char16_t part = compute_town_building_part(plan, position, building.direction);
                const BuildingPartType type = building_part_type(part);

                const gf::Vec2I map_position = town.position + block_position * (TownBuildingSize + StreetSize) + position;

                switch (type) {
                  case BuildingPartType::None:
                  case BuildingPartType::Outside:
                  case BuildingPartType::Furniture:
                    // nothing to do
                    break;
                  case BuildingPartType::Wall:
                    map.ground(map_position).decoration = MapCellDecoration::Wall;
                    break;
                }
              }
            }
          }
        }
      }

    }

    /*
     * Generate localities
     */

    void generate_localities(MapState& map, const WorldPlaces& places, [[maybe_unused]] gf::Random* random)
    {
      for (auto [ index, locality ] : gf::enumerate(map.localities)) {
        locality.position = to_map(places.localities[index].center);
        locality.type = places.localities[index].type;
        locality.number = 0; // TODO: make it random when available
        locality.direction = static_cast<gf::Direction>(random->compute_uniform_integer(3));
      }

      // put walls

      for (const LocalityState& locality : map.localities) {
        const LocalityBuildingPlan& plan = compute_locality_building_plan(locality.type, locality.number);
        const gf::Vec2I base_position = locality.position - LocalityRadius;

        for (int32_t y = 0; y < LocalityDiameter; ++y) {
          for (int32_t x = 0; x < LocalityDiameter; ++x) {
            const gf::Vec2I position = { x, y };
            const char16_t part = compute_locality_building_part(plan, position, locality.direction);
            const BuildingPartType type = building_part_type(part);

            const gf::Vec2I map_position = base_position + position;

            switch (type) {
              case BuildingPartType::None:
              case BuildingPartType::Outside:
              case BuildingPartType::Furniture:
                // nothing to do
                break;
              case BuildingPartType::Wall:
                map.ground(map_position).decoration = MapCellDecoration::Wall;
                break;
            }
          }
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

      std::vector<WorldRegion> trash_regions;

      std::vector<WorldRegion>& operator()(MapCellBiome region)
      {
        switch (region) {
          case MapCellBiome::Prairie:
            return prairie_regions;
          case MapCellBiome::Desert:
            return desert_regions;
          case MapCellBiome::Forest:
            return forest_regions;
          case MapCellBiome::Mountain:
            return mountain_regions;
          default:
            break;
        }

        return trash_regions;
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

      for (const gf::Vec2I position : state.ground.position_range()) {
        if (status(position) == Status::Visited) {
          continue;
        }

        const MapCellBiome region_type = state.ground(position).region;

        std::queue<gf::Vec2I> queue;
        queue.emplace(position);
        status(position) = Status::Visited;

        WorldRegion region = {};

        while (!queue.empty()) {
          const gf::Vec2I current = queue.front();
          queue.pop();
          assert(status(current) == Status::Visited);
          region.points.push_back(current);

          for (const gf::Vec2I neighbor : state.ground.compute_4_neighbors_range(current)) {
            if (status(neighbor) == Status::Visited) {
              continue;
            }

            if (state.ground(neighbor).region != region_type) {
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

    gf::Image compute_image_add_cave_accesses(const gf::Image& original, const MapState& state)
    {
      gf::Image image(original);

      for (const gf::Vec2I position : image.position_range()) {
        const MapCell& cell = state.ground(position);

        switch (cell.decoration) {
          case MapCellDecoration::FloorDown:
            image.put_pixel(position, gf::Yellow);
            break;
          default:
            break;
        }

      }

      return image;
    }

    gf::Image compute_underground_image_add_cave_accesses(const gf::Image& original, const MapState& state)
    {
      gf::Image image(original);

      for (const gf::Vec2I position : image.position_range()) {
        const MapCell& cell = state.underground(position);

        switch (cell.decoration) {
          case MapCellDecoration::FloorUp:
            image.put_pixel(position, gf::Orange);
            break;
          default:
            break;
        }

      }

      return image;
    }

    CaveAccess compute_underground_cave_access(MapState& state, const WorldRegion& region, gf::Random* random)
    {
      for (;;) {
        const std::size_t index = random->compute_uniform_integer(region.points.size());
        const gf::Vec2I entrance = region.points[index];

        if (is_on_side(entrance) || state.ground(entrance).decoration != MapCellDecoration::Cliff) {
          continue;
        }

        for (const gf::Vec2I exit : state.ground.compute_4_neighbors_range(entrance)) {
          if (!is_on_side(exit) && state.ground(exit).decoration != MapCellDecoration::Cliff) {
            return { entrance, exit };
          }
        }
      }

      return {{ 0, 0 }, { 0, 0 }};
    }

    std::vector<CaveAccess> compute_underground_cave_accesses(MapState& state, const WorldRegion& region, gf::Random* random)
    {
      std::size_t access_count = 1 + region.points.size() / SurfacePerCave;
      std::vector<CaveAccess> accesses(access_count);

      std::size_t tries = 0;

      // gf::Log::debug("\taccess count: {}", access_count);

      for (;;) {

        for (std::size_t i = 0; i < access_count; ++i) {
          accesses[i] = (compute_underground_cave_access(state, region, random));
        }

        if (access_count == 1) {
          return accesses;
        }

        int32_t min_distance = std::numeric_limits<int32_t>::max();

        for (std::size_t i = 0; i < access_count; ++i) {
          for (std::size_t j = i + 1; j < access_count; ++j) {
            const int32_t distance = gf::manhattan_distance(accesses[i].entrance, accesses[j].entrance);
            min_distance = std::min(min_distance, distance);
          }
        }

        // gf::Log::debug("\tmin distance: {}", min_distance);

        if (min_distance >= CaveMinDistance) {
          return accesses;
        }

        ++tries;

        if (tries > MaxCaveAccessTries) {
          --access_count;
          accesses.pop_back();
          tries = 0;
        }

      }

      return {};
    }


    std::vector<gf::Vec2I> compute_tunnel(gf::Vec2I from, gf::Vec2I to, gf::Random* random) {
      constexpr std::size_t Iterations = 5;
      constexpr gf::RectI Limits = gf::RectI::from_size(WorldSize).shrink_by(5);

      const std::size_t size = std::size_t(1) << Iterations;
      const std::size_t count = size + 1;

      std::vector<gf::Vec2I> points(count);
      points[0] = from;
      points[count - 1] = to;

      gf::Vec2I direction = gf::perp(from - to);

      std::size_t step = size / 2;

      while (step > 0) {
        for (std::size_t i = step; i < size; i += 2 * step) {
          assert(i - step < count);
          const gf::Vec2I prev = points[i - step];
          assert(i + step < count);
          const gf::Vec2I next = points[i + step];

          gf::Vec2I middle = (prev + next) / 2;
          middle += random->compute_uniform_float(-0.5f, +0.5f) * direction;
          points[i] = gf::clamp(middle, Limits.offset, Limits.offset + Limits.extent);
        }

        direction /= 2;
        step /= 2;
      }

      std::vector<gf::Vec2I> tunnel;

      for (std::size_t i = 0; i < points.size() - 1; ++i) {
        std::vector<gf::Vec2I> part = gf::generate_line(points[i], points[i + 1]);
        tunnel.insert(tunnel.end(), part.begin(), part.end());
      }

      return tunnel;
    }

    void compute_and_dig_tunnel(MapState& state, gf::Vec2I from, gf::Vec2I to, gf::Random* random) {
      constexpr gf::RectI Limits = gf::RectI::from_size(WorldSize).shrink_by(1);

      const std::vector<gf::Vec2I> tunnel = compute_tunnel(from, to, random);

      for (const gf::Vec2I position : tunnel) {
        state.underground(position).decoration = MapCellDecoration::None;

        for (const gf::Vec2I neighbor : state.underground.compute_8_neighbors_range(position)) {
          if (Limits.contains(neighbor)) {
            MapCell& cell = state.underground(neighbor);

            if (cell.decoration == MapCellDecoration::Rock) {
              cell.decoration = MapCellDecoration::None;
            }
          }
        }
      }
    }

    void compute_underground(MapState& state, const WorldRegions& regions, gf::Random* random)
    {
      state.underground = { WorldSize, { MapCellBiome::Underground, MapCellProperty::None, MapCellDecoration::Rock } };

      for (const WorldRegion& region : regions.mountain_regions) {
        const std::vector<CaveAccess> accesses = compute_underground_cave_accesses(state, region, random);

        for (const auto [ entrance, exit ] : accesses) {
          for (const gf::Vec2I cave : state.underground.compute_8_neighbors_range(exit)) {
            MapCell& cell = state.underground(cave);
            cell.decoration = MapCellDecoration::None;
          }

          state.underground(exit).decoration = MapCellDecoration::FloorUp;
          state.ground(entrance).decoration = MapCellDecoration::FloorDown;
        }

        const std::size_t access_count = accesses.size();

        auto compute_fake_entrance = [random](gf::Vec2I from) {
          const float radius = random->compute_radius(0.5f * CaveLinkDistance, 0.8f * CaveLinkDistance);
          const float angle = random->compute_angle();
          const gf::Vec2I fake_entrance = radius * gf::unit(angle);
          return from + fake_entrance;
        };

        constexpr gf::RectI Limits = gf::RectI::from_size(WorldSize).shrink_by(5);

        for (const auto [ entrance, exit ] : accesses) {
          const gf::Vec2I fake_entrance = compute_fake_entrance(entrance);

          if (Limits.contains(fake_entrance)) {
            compute_and_dig_tunnel(state, entrance, fake_entrance, random);
          }
        }

        if (access_count == 1) {
          const gf::Vec2I entrance = accesses.front().entrance;

          const gf::Vec2I fake_entrance = compute_fake_entrance(entrance);

          if (Limits.contains(fake_entrance)) {
            compute_and_dig_tunnel(state, entrance, fake_entrance, random);
          }

          break;
        }

        for (std::size_t i = 0; i < access_count; ++i) {
          for (std::size_t j = i + 1; j < access_count; ++j) {
            const gf::Vec2I entrance0 = accesses[i].entrance;
            const gf::Vec2I entrance1 = accesses[j].entrance;

            if (gf::manhattan_distance(entrance0, entrance1) < CaveLinkDistance) {
              compute_and_dig_tunnel(state, entrance0, entrance1, random);
            }
          }
        }
      }

      if constexpr (Debug) {
        gf::Image image = compute_basic_image(state.ground, ImageType::Blocks);
        // image = compute_image_add_towns_and_localities(image, places);
        // image = compute_image_add_railways(image, network);
        image = compute_image_add_cave_accesses(image, state);
        image.save_to_file("06_accesses.png");

        gf::Image underground_image = compute_basic_image(state.underground, ImageType::Blocks);
        underground_image = compute_underground_image_add_cave_accesses(underground_image, state);
        underground_image.save_to_file("06_access_underground.png");
      }

    }

    /*
     * Actors
     */

    enum class Seat {
      Free,
      Occupied
    };

    using SeatMap = gf::Array2D<Seat>;

    SeatMap compute_initial_seat_map(const WorldState& state)
    {
      SeatMap seat_map(state.map.ground.size(), Seat::Free);

      for (const TownState& town : state.map.towns) {
        for (const gf::Vec2I offset : gf::position_range({ TownDiameter, TownDiameter })) {
          seat_map(town.position + offset) = Seat::Occupied;
        }
      }

      for (const LocalityState& locality : state.map.localities) {
        for (const gf::Vec2I offset : gf::position_range({ LocalityDiameter, LocalityDiameter })) {
          seat_map(locality.position + offset) = Seat::Occupied;
        }
      }

      constexpr gf::RectI network_area = gf::RectI::from_center_size({ 0, 0 },{ ReducedFactor, ReducedFactor });

      for (const gf::Vec2I position : state.network.railway) {
        for (const gf::Vec2I offset : gf::rectangle_range(network_area)) {
          seat_map(position + offset) = Seat::Occupied;
        }
      }

      return seat_map;
    }

    /*
     * Animals
     */

    gf::Vec2I compute_animal_valid_position(const WorldState& state, const WorldRegion& region, const SeatMap& seat_map, gf::Random* random)
    {
      const BackgroundMap& background_map = state.map.from_floor(Floor::Ground); // TODO: parameter?

      for (;;) {
        const std::size_t position_index = random->compute_uniform_integer(region.points.size());
        assert(position_index < region.points.size());

        const gf::Vec2I position = region.points[position_index];

        if (!fw::is_walkable(background_map(position).decoration)) {
          continue;
        }

        if (seat_map(position) == Seat::Occupied) {
          continue;
        }

        return position;
      }

      return {};
    }

    void compute_animals_in_regions(WorldState& state, const WorldData& data, const std::vector<WorldRegion>& regions, SeatMap& seat_map, gf::Random* random, std::string_view name, std::size_t density)
    {
      std::size_t overall_count = 0;

      for (const WorldRegion& region : regions) {
        const std::size_t count = region.points.size() / density + 1;
        overall_count += count;

        for (std::size_t i = 0; i < count; ++i) {
          const gf::Vec2I position = compute_animal_valid_position(state, region, seat_map, random);
          seat_map(position) = Seat::Occupied;

          ActorState animal = generate_animal(name, { position, Floor::Ground }, data, random);

          const uint32_t id = static_cast<uint32_t>(state.actors.size());
          state.actors.push_back(animal);

          state.scheduler.queue.push({state.current_date, id});
        }
      }

      gf::Log::info("\t{}: {}", name, overall_count);
    }

    void compute_animals(WorldState& state, const WorldData& data, const WorldRegions& regions, SeatMap& seat_map, gf::Random* random)
    {
      compute_animals_in_regions(state, data, regions.desert_regions, seat_map, random, "Snake", 1000);
      compute_animals_in_regions(state, data, regions.mountain_regions, seat_map, random, "Scorpion", 1000);
      compute_animals_in_regions(state, data, regions.prairie_regions, seat_map, random, "Coyote", 1500);
      compute_animals_in_regions(state, data, regions.forest_regions, seat_map, random, "Grizzli", 1500);
    }


    /*
     * Hero
     */

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

    ActorState generate_hero(const WorldState& state, const WorldData& data, gf::Random* random)
    {
      const gf::Vec2I position = compute_starting_position(state.network);
      ActorState hero = generate_human("Hero", { position, Floor::Ground }, data, random);

      HumanComponent& component = hero.component.from<ActorType::Human>();
      component.weapon.data = "Smith & Wesson Model 3 Revolver";

      DistanceWeaponComponent weapon_component;
      weapon_component.projectiles = 3;
      component.weapon.component = weapon_component;

      component.projectile.data = ".44 Ammunition";
      component.projectile.count = 10;

      InventoryItemState item;
      item.count = 1;
      item.data = "Colt Walker Revolver";
      component.inventory.items.push_back(item);

      item.data = "LeMat Revolver";
      component.inventory.items.push_back(item);

      item.data = "Knife";
      component.inventory.items.push_back(item);

      item.data = ".44 Ammunition";
      item.count = 40;
      component.inventory.items.push_back(item);

      return hero;
    }

  }

  WorldState generate_world(gf::Random* random, const WorldData& data, WorldGenerationAnalysis& analysis)
  {
    gf::Clock clock;

    WorldState state = {};
    analysis.set_step(WorldGenerationStep::Date);
    state.current_date = Date::generate_random(random);

    gf::Log::info("Starting generation...");
    analysis.set_step(WorldGenerationStep::Terrain);
    const RawWorld primitive_raw = generate_raw(random);
    gf::Log::info("- raw ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Rivers);
    const std::vector<River> rivers = generate_rivers(primitive_raw, random);
    const RawWorld raw = modify_raw_with_rivers(primitive_raw, rivers, random);
    gf::Log::info("- rivers ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Biomes);
    state.map = generate_outline(raw, random, rivers);
    gf::Log::info("- outline ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Moutains);
    generate_mountains(state.map, random);
    gf::Log::info("- moutains ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Towns);
    const WorldPlaces places = generate_places(state.map, random);
    gf::Log::info("- places ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Rails);
    state.network = generate_network(raw, state.map, places, random);
    gf::Log::info("- network ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Roads);
    generate_roads(raw, state.map, state.network, places);
    gf::Log::info("- roads ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Buildings);
    generate_towns(state.map, places, random);
    generate_localities(state.map, places, random);
    gf::Log::info("- towns and localities ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Regions);
    const WorldRegions regions = compute_regions(state.map);
    gf::Log::info("- regions ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Underground);
    compute_underground(state.map, regions, random);
    gf::Log::info("- underground ({:g}s)", clock.elapsed_time().as_seconds());

    analysis.set_step(WorldGenerationStep::Hero);

    {
      ActorState hero = generate_hero(state, data, random);
      compute_hero_fov(hero.location().position, state.map.ground);

      assert(state.actors.empty());
      state.actors.push_back(hero);

      Date next_turn = state.current_date;
      next_turn.add_seconds(1);
      state.scheduler.queue.push({next_turn, 0});

    }

    analysis.set_step(WorldGenerationStep::Actors);

    // add the trains: at the beginning, one train arriving in each station

    for (const StationState& station : state.network.stations) {
      Date date = state.current_date;
      date.add_seconds(station.stop_time);

      ActorState train = {};
      train.data = "Train";

      TrainComponent component;
      component.railway_index = station.index;
      train.component = component;

      const uint32_t index = static_cast<uint32_t>(state.actors.size());
      state.actors.push_back(train);

      state.scheduler.queue.push({ .date = date, .index = index });
    }

    SeatMap seat_map = compute_initial_seat_map(state);
    compute_animals(state, data, regions, seat_map, random);

    gf::Log::info("actors: {}", state.actors.size());

    // state.add_message(fmt::format("Hello <style=character>{}</>!", human.name));

    gf::Log::info("- actors ({:g}s)", clock.elapsed_time().as_seconds());

    return state;
  }

}
