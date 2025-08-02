#include "MapElement.h"

#include <cstdint>
#include <string_view>

#include <gf2/core/Direction.h>

#include "ActorState.h"
#include "FarFarWest.h"
#include "MapRuntime.h"
#include "NetworkState.h"
#include "Pictures.h"
#include "Settings.h"
#include "Utils.h"

namespace ffw {

  namespace {

    constexpr int32_t ViewRelaxation = 5;

    constexpr int32_t TrainSize = 3;
    using RailPart = std::array<std::u16string_view, TrainSize>;

    constexpr RailPart LocomotiveFront = {{
      u" ▄ ",
      u"▐◘▌",
      u"▐█▌",
    }};

    constexpr RailPart LocomotiveBack = {{
      u"▐█▌",
      u"███",
      u"███",
    }};

    constexpr RailPart Coupler = {{
      u"▐█▌",
      u" · ",
      u"▐█▌",
    }};

    constexpr RailPart WagonFront = {{
      u"███",
      u"███",
      u"███",
    }};

    constexpr RailPart WagonBack = {{
      u"███",
      u"███",
      u"███",
    }};

    constexpr std::string_view Train = "lLCwWCwWCwW";
    static_assert(Train.length() == TrainLength);

    const RailPart& compute_train_basic_part(char part_character)
    {
      switch (part_character) {
        case 'l':
          return LocomotiveFront;
        case 'L':
          return LocomotiveBack;
        case 'C':
          return Coupler;
        case 'w':
          return WagonFront;
        case 'W':
          return WagonBack;
      }

      assert(false);
      return Coupler;
    }

    char16_t compute_train_part(const RailPart& part, gf::Vec2I position, gf::Direction direction)
    {
      assert(0 <= position.x && position.x < TrainSize);
      assert(0 <= position.y && position.y < TrainSize);

      char16_t picture = u'#';

      switch (direction) {
        case gf::Direction::Up:
          picture = part[position.y][position.x];
          break;
        case gf::Direction::Right:
          picture = part[TrainSize - position.x - 1][position.y];
          break;
        case gf::Direction::Down:
          picture = part[TrainSize - position.y - 1][TrainSize - position.x - 1];
          break;
        case gf::Direction::Left:
          picture = part[position.x][TrainSize - position.y - 1];
          break;
        default:
          assert(false);
          break;
      }

      return rotate_picture(picture, direction);
    }

  }

  MapElement::MapElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void MapElement::update([[maybe_unused]] gf::Time time)
  {
    const WorldState* state = m_game->state();
    WorldRuntime* runtime = m_game->runtime();
    const gf::Vec2I hero_position = state->hero().position;
    runtime->view_center = gf::clamp(runtime->view_center, hero_position - ViewRelaxation, hero_position + ViewRelaxation);
  }

  void MapElement::render(gf::Console& console)
  {
    const ActorState& hero = m_game->state()->hero();

    // get current view

    const WorldRuntime* runtime = m_game->runtime();
    const gf::RectI view = runtime->compute_view();

    // display map background

    const FloorMap& floor_map = runtime->map.from_floor(hero.floor);
    floor_map.console.blit_to(console, view, GameBoxPosition);

    // display actors

    const WorldState* state = m_game->state();

    gf::ConsoleStyle actor_style;

    for (const ActorState& actor : state->actors) {
      if (!view.contains(actor.position)) {
        continue;
      }

      if (actor.floor != hero.floor) {
        continue;
      }

      actor_style.color.background = gf::Transparent;
      actor_style.color.foreground = actor.data->color;
      actor_style.effect = gf::ConsoleEffect::none();
      char16_t actor_picture = actor.data->picture;

      if (actor.feature.type() == ActorType::Animal) {
        const uint32_t index = actor.feature.from<ActorType::Animal>().mounted_by;

        if (index != NoIndex) {
          const ActorState& mounted_by = state->actors[index];
          actor_style.color.background = mounted_by.data->color;
          actor_style.effect = gf::ConsoleEffect::alpha(0.2f);
          actor_picture = to_uppercase_ascii(actor_picture);
        }
      }

      console.put_character(actor.position - view.position(), actor_picture, actor_style);
    }

    // display trains

    if (hero.floor != Floor::Ground) {
      return;
    }

    gf::ConsoleStyle train_style;
    train_style.color.foreground = gf::gray(0.1f);
    train_style.color.background = gf::Transparent;
    train_style.effect = gf::ConsoleEffect::none();

    for (const TrainState& train : state->network.trains) {
      uint32_t offset = 0;

      for (char part_character : Train) {
        const uint32_t index = runtime->network.next_position(train.railway_index, offset);
        assert(index < runtime->network.railway.size());
        const gf::Vec2I position = runtime->network.railway[index];

        const uint32_t next_index = runtime->network.prev_position(index);
        assert(next_index < runtime->network.railway.size());
        const gf::Vec2I next_position = runtime->network.railway[next_index];

        assert(gf::manhattan_distance(position, next_position) == 1);
        const gf::Direction direction = undisplacement(next_position - position);

        const RailPart& part = compute_train_basic_part(part_character);

        for (int32_t i = -1; i <= 1; ++i) {
          for (int32_t j = -1; j <= 1; ++j) {
            const gf::Vec2I neighbor = { i, j };
            const gf::Vec2I neighbor_position = position + neighbor;

            if (!view.contains(neighbor_position)) {
              continue;
            }

            console.put_character(neighbor_position - view.position(), compute_train_part(part, neighbor + 1, direction), train_style);
          }
        }

        offset += 3;
      }
    }

  }

}
