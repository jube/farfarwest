#include "MapRuntime.h"

#include <gf2/core/ConsoleChar.h>

#include "MapCell.h"
#include "Settings.h"
#include "WorldState.h"

namespace ffw {

  namespace {

    constexpr gf::Color PrairieColor = 0xC4D6B0;
    constexpr gf::Color DesertColor = 0xC2B280;
    constexpr gf::Color ForestColor = 0x4A6A4D;
    constexpr gf::Color MountainColor = 0x8B5A2B;

    constexpr float ColorLighterBound = 0.03f;

    char16_t generate_character(std::initializer_list<char16_t> list, gf::Random* random)
    {
      assert(list.size() > 0);
      const std::size_t index = random->compute_uniform_integer(list.size());
      assert(index < list.size());
      return std::data(list)[index];
    }

  }

  void MapRuntime::bind(const WorldState& state, gf::Random* random)
  {
    outside_ground = gf::Console(WorldSize);
    outside_grid = gf::GridMap::make_orthogonal(WorldSize);
    outside_reverse = gf::Array2D<ReverseMapCell>(WorldSize);

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
            uint8_t neighbor_bits = 0b0000;
            uint8_t direction_bit = 0b0001;

            for (const gf::Orientation orientation : { gf::Orientation::North, gf::Orientation::East, gf::Orientation::South, gf::Orientation::West }) {
              const gf::Vec2I target = position + gf::displacement(orientation);

              if (!state.map.cells.valid(target) || state.map.cells(target).block == MapBlock::Cliff) {
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
      }

      outside_ground.put_character(position, character, foreground_color, background_color);

      if (cell.block != MapBlock::None) {
        outside_grid.set_walkable(position, false);
        outside_grid.set_transparent(position, false);
      }
    }

    for (const auto& [ index, actor ] : gf::enumerate(state.actors)) {
      outside_reverse(actor.position).actor_index = index;
    }

  }

}
