#include "MapRuntime.h"

#include <gf2/core/ConsoleChar.h>

#include "Settings.h"
#include "WorldState.h"

namespace ffw {

  namespace {

    char16_t generate_character(std::initializer_list<char16_t> list, float state)
    {
      assert(list.size() > 0);
      assert(0.0f <= state && state < 1.0f);
      const std::size_t index = static_cast<std::size_t>(state * list.size());
      assert(index < list.size());
      return std::data(list)[index];
    }

  }

  void MapRuntime::bind(const WorldState& state)
  {
    outside_ground = gf::Console(WorldSize);
    outside_grid = gf::GridMap::make_orthogonal(WorldSize);
    outside_reverse = gf::Array2D<ReverseMapCell>(WorldSize);

    for (auto position : state.map.cells.position_range()) {
      const MapCell& cell = state.map.cells(position);

      gf::Color background_color = cell.trait.background;
      gf::Color foreground_color = gf::Transparent;
      char16_t character = u' ';

      switch (cell.detail.block) {
        case MapBlock::None:
          break;
        case MapBlock::Cactus:
          character = generate_character({ u'!', gf::ConsoleChar::InvertedExclamationMark }, cell.detail.state);
          foreground_color = gf::darker(gf::Green, 0.3f);
          break;
        case MapBlock::Tree:
          character = generate_character({ gf::ConsoleChar::GreekPhiSymbol, gf::ConsoleChar::YenSign }, cell.detail.state);
          foreground_color = gf::darker(gf::Green, 0.7f);
          break;
        case MapBlock::Cliff:
          {
            uint8_t neighbor_bits = 0b0000;
            uint8_t direction_bit = 0b0001;

            for (const gf::Orientation orientation : { gf::Orientation::North, gf::Orientation::East, gf::Orientation::South, gf::Orientation::West }) {
              const gf::Vec2I target = position + gf::displacement(orientation);

              if (!state.map.cells.valid(target) || state.map.cells(target).detail.block == MapBlock::Cliff) {
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

      switch (cell.detail.decoration) {
        case MapDecoration::None:
          break;
        case MapDecoration::Herb:
          character = generate_character({ u'.', u',', u'`', u'\'' /*, gf::ConsoleChar::SquareRoot */ }, cell.detail.state);
          foreground_color = gf::darker(background_color, 0.1f);
          break;
      }

      outside_ground.put_character(position, character, foreground_color, background_color);

      if (cell.detail.block != MapBlock::None) {
        outside_grid.set_walkable(position, false);
        outside_grid.set_transparent(position, false);
      }
    }

    for (const auto& [ index, actor ] : gf::enumerate(state.actors)) {
      outside_reverse(actor.position).actor_index = index;
    }

  }

}
