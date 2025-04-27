#include "MapRuntime.h"

#include <cstdint>

#include <gf2/core/ConsoleChar.h>
#include <gf2/core/Direction.h>

#include "Colors.h"
#include "MapCell.h"
#include "MapState.h"
#include "NetworkState.h"
#include "Settings.h"
#include "Utils.h"
#include "WorldState.h"

namespace ffw {

  namespace {

    constexpr float ColorLighterBound = 0.03f;

    char16_t generate_character(std::initializer_list<char16_t> list, gf::Random* random)
    {
      assert(list.size() > 0);
      const std::size_t index = random->compute_uniform_integer(list.size());
      assert(index < list.size());
      return std::data(list)[index];
    }

    template<typename T, T MapCell::* Field>
    uint8_t compute_neighbor_bits(const MapState& state, gf::Vec2I position, T type)
    {
      uint8_t neighbor_bits = 0b0000;
      uint8_t direction_bit = 0b0001;

      for (const gf::Orientation orientation : { gf::Orientation::North, gf::Orientation::East, gf::Orientation::South, gf::Orientation::West }) {
        const gf::Vec2I target = position + gf::displacement(orientation);

        if (!state.cells.valid(target) || state.cells(target).*Field == type) {
          neighbor_bits |= direction_bit;
        }

        direction_bit <<= 1;
      }

      return neighbor_bits;
    }

    using BuildingPlan = std::array<std::u16string_view, BuildingSize>;

    constexpr BuildingPlan Saloon = {{
      u"╔═══════╦═╗",
      u"║       ║ ║",
      u"╟─────┤ ╨ ║",
      u"║  · ·    ║",
      u"║ ·     · ║",
      u"║·•·   ·•·║",
      u"║ ·     · ║",
      u"║  ·   ·  ║",
      u"║ ·•· ·•· ║",
      u"║  ·   ·  ║",
      u"╚════─════╝",
    }};

    constexpr BuildingPlan Template = {{
      u"╔═════════╗",
      u"║         ║",
      u"║         ║",
      u"║         ║",
      u"║         ║",
      u"║         ║",
      u"║         ║",
      u"║         ║",
      u"║         ║",
      u"║         ║",
      u"╚════─════╝",
    }};


    char16_t compute_building_part(const BuildingPlan& building, gf::Vec2I position, gf::Direction entrance)
    {
      assert(0 <= position.x && position.x < BuildingSize);
      assert(0 <= position.y && position.y < BuildingSize);

      switch (entrance) {
        case gf::Direction::Up:
          return building[BuildingSize - position.x - 1][BuildingSize - position.y - 1];
        case gf::Direction::Left:
          return building[BuildingSize - position.y - 1][position.x];
        case gf::Direction::Down:
          return building[position.x][position.y];
        case gf::Direction::Right:
          return building[position.y][BuildingSize - position.x - 1];
        default:
          break;
      }

      return building[position.x][position.y];
    }

  }

  void MapRuntime::bind(const WorldState& state, gf::Random* random)
  {
    outside_ground = gf::Console(WorldSize);
    outside_grid = gf::GridMap::make_orthogonal(WorldSize);
    outside_reverse = gf::Array2D<ReverseMapCell>(WorldSize);

    bind_ground(state, random);
    bind_railway(state);
    bind_towns(state);
    bind_reverse(state);
  }

  void MapRuntime::bind_ground(const WorldState& state, gf::Random* random)
  {
    for (auto position : state.map.cells.position_range()) {
      const MapCell& cell = state.map.cells(position);

      gf::Color background_color = gf::White;

      switch (cell.region) {
        case MapRegion::Prairie:
          background_color = PrairieColor;
          break;
        case MapRegion::Desert:
          background_color = DesertColor;
          break;
        case MapRegion::Forest:
          background_color = ForestColor;
          break;
        case MapRegion::Moutain:
          background_color = MountainColor;
          break;
      }

      background_color = gf::lighter(background_color, random->compute_uniform_float(0.0f, ColorLighterBound));

      gf::Color foreground_color = gf::Transparent;
      char16_t character = u' ';

      switch (cell.block) {
        case MapBlock::None:
          break;
        case MapBlock::Cactus:
          character = generate_character({ u'!', gf::ConsoleChar::InvertedExclamationMark }, random);
          foreground_color = gf::darker(gf::Green, 0.3f);
          break;
        case MapBlock::Tree:
          character = generate_character({ gf::ConsoleChar::GreekPhiSymbol, gf::ConsoleChar::YenSign }, random);
          foreground_color = gf::darker(gf::Green, 0.7f);
          break;
        case MapBlock::Cliff:
          {
            const uint8_t neighbor_bits = compute_neighbor_bits<MapBlock, &MapCell::block>(state.map, position, MapBlock::Cliff);

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
            foreground_color = gf::darker(background_color, 0.5f);
          }
          break;
      }

      switch (cell.decoration) {
        case MapDecoration::None:
          break;
        case MapDecoration::Herb:
          character = generate_character({ u'.', u',', u'`', u'\'' /*, gf::ConsoleChar::SquareRoot */ }, random);
          foreground_color = gf::darker(background_color, 0.1f);
          break;
        case MapDecoration::Rail:
          break;
      }

      outside_ground.put_character(position, character, foreground_color, background_color);

      if (cell.block != MapBlock::None) {
        outside_grid.set_walkable(position, false);
        outside_grid.set_transparent(position, false);
      }
    }
  }

  using RailPlan = std::array<std::u16string_view, 3>;

  constexpr RailPlan RailNS = {{
    u"│ │",
    u"┼─┼",
    u"│ │",
  }};

  constexpr RailPlan RailNW = {{
    u"┘ │",
    u"  │",
    u"──┘",
  }};

  constexpr RailPlan RailNE = {{
    u"│ └",
    u"│  ",
    u"└──",
  }};

  constexpr RailPlan RailWE = {{
    u"─┼─",
    u" │ ",
    u"─┼─",
  }};

  constexpr RailPlan RailSW = {{
    u"──┐",
    u"  │",
    u"┐ │",
  }};

  constexpr RailPlan RailSE = {{
    u"┌──",
    u"│  ",
    u"│ ┌",
  }};

  uint8_t direction_bit(gf::Direction direction)
  {
    assert(direction != gf::Direction::Center);
    return 1 << static_cast<int8_t>(direction);
  }

  const RailPlan& compute_rail_plan(gf::Direction direction_before, gf::Direction direction_after)
  {
    const uint8_t bits = direction_bit(direction_before) | direction_bit(direction_after);

    switch (bits) {
      //     WSEN
      case 0b0011: return RailNE;
      case 0b0101: return RailNS;
      case 0b0110: return RailSE;
      case 0b1001: return RailNW;
      case 0b1010: return RailWE;
      case 0b1100: return RailSW;
      default:
        assert(false);
        break;
    }

    return RailNS;
  }


  void MapRuntime::bind_railway(const WorldState& state)
  {
    gf::ConsoleStyle style;
    style.color.foreground = gf::Black;
    style.color.background = gf::Transparent;
    style.effect = gf::ConsoleEffect::none();

    const std::vector<gf::Vec2I> railway = state.network.railway;

    // put railway on the map

    for (const auto [ index, position ] : gf::enumerate(railway)) {
      const std::size_t index_before = (index + railway.size() - 1) % railway.size();
      const gf::Vec2I position_before = railway[index_before];
      const gf::Direction direction_before = undisplacement(gf::sign(position_before - position));

      const std::size_t index_after = (index + 1) % railway.size();
      const gf::Vec2I position_after = railway[index_after];
      const gf::Direction direction_after = undisplacement(gf::sign(position_after - position));

      const RailPlan& plan = compute_rail_plan(direction_before, direction_after);

      for (int i = -1; i <= +1; ++i) {
        for (int j = -1; j <= +1; ++j) {
          const gf::Vec2I neighbor(i, j);
          const gf::Vec2I neighbor_position = position + neighbor;

          outside_ground.put_character(neighbor_position, plan[neighbor.y + 1][neighbor.x + 1], style);
        }
      }
    }
  }

  void MapRuntime::bind_towns(const WorldState& state)
  {
    for (const TownState& town : state.map.towns) {
      for (int32_t i = 0; i < TownsBlockSize; ++i) {
        for (int32_t j = 0; j < TownsBlockSize; ++j) {
          if (town.buildings[i][j] == Building::Empty || town.buildings[i][j] == Building::None) {
            continue;
          }

          const gf::Vec2I block_position = { j, i };

          for (int32_t x = 0; x < BuildingSize; ++x) {
            for (int32_t y = 0; y < BuildingSize; ++y) {
              const gf::Vec2I position = { x, y };
              const char16_t part = compute_building_part(Template, { y, x }, gf::Direction::Down);

              const gf::Vec2I map_position = town.position + block_position * (BuildingSize + StreetSize) + position;

              outside_ground.put_character(map_position, part, gf::Gray, gf::White);
            }
          }
        }
      }
    }
  }


  void MapRuntime::bind_reverse(const WorldState& state)
  {
    for (const auto& [ index, actor ] : gf::enumerate(state.actors)) {
      outside_reverse(actor.position).actor_index = index;
    }
  }

}
