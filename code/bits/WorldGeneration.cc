#include "WorldGeneration.h"

#include <cstdint>

#include <algorithm>
#include <queue>
#include <string_view>

#include <gf2/core/Array2D.h>
#include <gf2/core/Clock.h>
#include <gf2/core/Direction.h>
#include <gf2/core/Easing.h>
#include <gf2/core/Geometry.h>
#include <gf2/core/GridMap.h>
#include <gf2/core/Heightmap.h>
#include <gf2/core/Log.h>
#include <gf2/core/Noises.h>
#include <gf2/core/ProcGen.h>
#include <gf2/core/Vec2.h>

#include "ActorState.h"
#include "Colors.h"
#include "Date.h"
#include "MapCell.h"
#include "MapState.h"
#include "Names.h"
#include "Settings.h"
#include "gf2/core/Color.h"

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

    constexpr int32_t ReducedLocalityDiameter = LocalityDiameter / ReducedFactor;
    constexpr int32_t LocalityMinDistanceFromOther = 250;

    constexpr std::size_t DayTime = 24 * 60 * 60;
    constexpr int32_t RailSpacing = 2;

    constexpr int CliffThreshold = 2;

    constexpr float SlopeFactor = 225.0f;
    constexpr float RailBlockPenalty = 1.5f;
    constexpr float DoubleRailBlockPenalty = 5.0f;

    constexpr int32_t RoadMaxDistanceFromLocality = 300;
    constexpr int32_t RoadMaxDistanceFromTown = 350;
    static_assert(LocalityMinDistanceFromOther < RoadMaxDistanceFromLocality && RoadMaxDistanceFromTown < TownMinDistanceFromOther);

    constexpr std::size_t RegionMinimumSize = 400;

    constexpr gf::Vec2I EightNeighbors[] = {
      // clang-format off
      { -1, -1 }, {  0, -1 }, { +1, -1 },
      { -1,  0 },             { +1,  0 },
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

    gf::Image compute_basic_underground_image(const MapState& state) {
      gf::Image image(WorldSize);

      for (const gf::Vec2I position : image.position_range()) {
        const MapUndergroundCell& cell = state.underground(position);
        gf::Color color = gf::Transparent;

        switch (cell.type) {
          case MapUnderground::Rock:
            color = RockColor;
            break;
          case MapUnderground::Dirt:
            color = DirtColor;
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

    float distance_with_slope(const RawWorld& raw, gf::Vec2I position, gf::Vec2I neighbor)
    {
      const float distance = gf::euclidean_distance<float>(position, neighbor);
      const float slope = static_cast<float>(std::abs(raw(to_map(position)).altitude - raw(to_map(neighbor)).altitude)) / distance;
      return distance * (1 + SlopeFactor * gf::square(slope));
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
        gf::Image image = compute_basic_image(state, ImageType::Blocks);
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

        for (const gf::Vec2I neighbor : state.cells.compute_24_neighbors_range(map_position)) {
          if (state.cells(neighbor).decoration == MapDecoration::Cliff) {
            ++cliffs;
          }
        }

        grid.set_walkable(position, cliffs <= CliffThreshold);
      }

      return grid;
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
          return distance_with_slope(raw, position, neighbor);
        });

        for (const gf::Vec2I point : path) {
          grid.set_walkable(point, false);

          for (const gf::Vec2I relative_neighbor : EightNeighbors) {
            grid.set_walkable(point + relative_neighbor, false);
          }
        }

        assert(!path.empty());
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
        const float distance = distance_with_slope(raw, position, neighbor);

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
        gf::Image image = compute_basic_image(state, ImageType::Blocks);
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
     * Generate localities
     */

    void generate_localities(MapState& map, const WorldPlaces& places, [[maybe_unused]] gf::Random* random)
    {
      for (auto [ index, locality ] : gf::enumerate(map.localities)) {
        locality.position = to_map(places.localities[index].center);
        locality.type = places.localities[index].type;
        locality.number = 0; // TODO: make it random when available
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

    gf::Image compute_image_add_cave_accesses(const gf::Image& original, const MapState& state)
    {
      gf::Image image(original);

      for (const gf::Vec2I position : image.position_range()) {
        const MapCell& cell = state.cells(position);

        switch (cell.decoration) {
          case MapDecoration::FloorDown:
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
        const MapUndergroundCell& cell = state.underground(position);

        switch (cell.decoration) {
          case MapDecoration::FloorUp:
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

        if (is_on_side(entrance) || state.cells(entrance).decoration != MapDecoration::Cliff) {
          continue;
        }

        for (const gf::Vec2I exit : state.cells.compute_4_neighbors_range(entrance)) {
          if (!is_on_side(exit) && state.cells(exit).decoration != MapDecoration::Cliff) {
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
       state.underground(position).type = MapUnderground::Dirt;

        for (const gf::Vec2I neighbor : state.underground.compute_8_neighbors_range(position)) {
          if (Limits.contains(neighbor)) {
            state.underground(neighbor).type = MapUnderground::Dirt;
          }
        }
      }
    }

    void compute_underground(MapState& state, const WorldRegions& regions, gf::Random* random)
    {
      state.underground = { WorldSize, { MapUnderground::Rock, MapDecoration::None } };

      for (const WorldRegion& region : regions.mountain_regions) {
        const std::vector<CaveAccess> accesses = compute_underground_cave_accesses(state, region, random);

        for (const auto [ entrance, exit ] : accesses) {
          state.underground(entrance).type = MapUnderground::Dirt;

          for (const gf::Vec2I cave : state.underground.compute_8_neighbors_range(entrance)) {
            MapUndergroundCell& cell = state.underground(cave);
            cell.type = MapUnderground::Dirt;
          }

          state.underground(exit).decoration = MapDecoration::FloorUp;
          state.cells(entrance).decoration = MapDecoration::FloorDown;
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
        gf::Image image = compute_basic_image(state, ImageType::Blocks);
        // image = compute_image_add_towns_and_localities(image, places);
        // image = compute_image_add_railways(image, network);
        image = compute_image_add_cave_accesses(image, state);
        image.save_to_file("06_accesses.png");

        gf::Image underground_image = compute_basic_underground_image(state);
        underground_image = compute_underground_image_add_cave_accesses(underground_image, state);
        underground_image.save_to_file("06_access_underground.png");
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

  WorldState generate_world(gf::Random* random, std::atomic<WorldGenerationStep>& step)
  {
    gf::Clock clock;

    WorldState state = {};
    step.store(WorldGenerationStep::Date);
    state.current_date = Date::generate_random(random);

    gf::Log::info("Starting generation...");
    step.store(WorldGenerationStep::Terrain);
    const RawWorld raw = generate_raw(random);
    gf::Log::info("- raw ({:g}s)", clock.elapsed_time().as_seconds());

    step.store(WorldGenerationStep::Biomes);
    state.map = generate_outline(raw, random);
    gf::Log::info("- outline ({:g}s)", clock.elapsed_time().as_seconds());

    step.store(WorldGenerationStep::Moutains);
    generate_mountains(state.map, random);
    gf::Log::info("- moutains ({:g}s)", clock.elapsed_time().as_seconds());

    step.store(WorldGenerationStep::Towns);
    const WorldPlaces places = generate_places(state.map, random);
    gf::Log::info("- places ({:g}s)", clock.elapsed_time().as_seconds());

    step.store(WorldGenerationStep::Rails);
    state.network = generate_network(raw, state.map, places, random);
    gf::Log::info("- network ({:g}s)", clock.elapsed_time().as_seconds());

    // TODO: step
    generate_roads(raw, state.map, state.network, places);
    gf::Log::info("- roads ({:g}s)", clock.elapsed_time().as_seconds());

    step.store(WorldGenerationStep::Buildings);
    generate_towns(state.map, places, random);
    generate_localities(state.map, places, random);
    gf::Log::info("- towns and localities ({:g}s)", clock.elapsed_time().as_seconds());

    step.store(WorldGenerationStep::Regions);
    const WorldRegions regions = compute_regions(state.map);
    gf::Log::info("- regions ({:g}s)", clock.elapsed_time().as_seconds());

    step.store(WorldGenerationStep::Underground);
    compute_underground(state.map, regions, random);
    gf::Log::info("- underground ({:g}s)", clock.elapsed_time().as_seconds());

    step.store(WorldGenerationStep::Hero);

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

    step.store(WorldGenerationStep::Actors);

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

    step.store(WorldGenerationStep::End);
    return state;
  }

}
