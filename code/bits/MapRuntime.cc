#include "MapRuntime.h"

#include <cstdint>
#include <gf2/core/ConsoleChar.h>

#include "Colors.h"
#include "MapCell.h"
#include "MapState.h"
#include "Settings.h"
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

    constexpr std::u16string_view Saloon[] = {
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
    };

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
          {
            const uint8_t neighbor_bits = compute_neighbor_bits<MapDecoration, &MapCell::decoration>(state.map, position, MapDecoration::Rail);

            constexpr char16_t RailCharacters[] = {
                                                    // WSEN
              u' ',                                 // 0000
              u' ',                                 // 0001
              u' ',                                 // 0010
              u'╚',                                 // 0011
              u' ',                                 // 0100
              u'╫',                                 // 0101
              u'╔',                                 // 0110
              u'╫',                                 // 0111
              u' ',                                 // 1000
              u'╝',                                 // 1001
              u'╪',                                 // 1010
              u'╪',                                 // 1011
              u'╗',                                 // 1100
              u'╫',                                 // 1101
              u'╪',                                 // 1110
              u' ',                                 // 1111
            };

            assert(neighbor_bits < std::size(RailCharacters));
            character = RailCharacters[neighbor_bits];
            assert(character != u' ');
            foreground_color = gf::Gray;
          }
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

    for (const auto& [ index, train ] : gf::enumerate(state.network.trains)) {
      for (uint32_t i = 0; i < TrainSize; ++i) {
        const uint32_t railway_index = state.network.next_position(train.index, i);
        assert(railway_index < state.network.railway.size());
        const gf::Vec2I position = state.network.railway[railway_index];
        outside_reverse(position).train_index = index;
      }
    }

  }

}
