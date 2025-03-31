#include "MapScene.h"

#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  namespace {
    using namespace gf::literals;

    struct MoveAction {
      gf::Id id;
      gf::Orientation orientation;
      gf::Scancode key;
      gf::Scancode alt_key;
    };

    constexpr MoveAction MoveActions[] {
      { "go_south_west"_id, gf::Orientation::SouthWest, gf::Scancode::Numpad1, gf::Scancode::Unknown },
      { "go_south"_id,      gf::Orientation::South,     gf::Scancode::Numpad2, gf::Scancode::Down },
      { "go_south_east"_id, gf::Orientation::SouthEast, gf::Scancode::Numpad3, gf::Scancode::Unknown },
      { "go_west"_id,       gf::Orientation::West,      gf::Scancode::Numpad4, gf::Scancode::Left },
      { "idle"_id,          gf::Orientation::Center,    gf::Scancode::Numpad5, gf::Scancode::Unknown },
      { "go_east"_id,       gf::Orientation::East,      gf::Scancode::Numpad6, gf::Scancode::Right },
      { "go_north_west"_id, gf::Orientation::NorthWest, gf::Scancode::Numpad7, gf::Scancode::Unknown },
      { "go_north"_id,      gf::Orientation::North,     gf::Scancode::Numpad8, gf::Scancode::Up },
      { "go_north_east"_id, gf::Orientation::NorthEast, gf::Scancode::Numpad9, gf::Scancode::Unknown },
    };

  }

  MapScene::MapScene(FarFarWest* game)
  : m_game(game)
  , m_action_group(compute_settings())
  {
  }

  void MapScene::process_event(const gf::Event& event)
  {
    m_action_group.process_event(event);
  }

  gf::ActionGroupSettings MapScene::compute_settings()
  {
    using namespace gf::literals;
    gf::ActionGroupSettings settings;

    for (auto move_action : MoveActions) {
      gf::ActionSettings action = gf::instantaneous_action().add_scancode_control(move_action.key);

      if (move_action.alt_key != gf::Scancode::Unknown) {
        action.add_scancode_control(move_action.alt_key);
      }

      settings.actions.emplace(move_action.id, std::move(action));
    }

    return settings;
  }

  void MapScene::handle_actions()
  {
    using namespace gf::literals;

    for (auto move_action : MoveActions) {
      if (m_action_group.active(move_action.id)) {
        m_orientation += gf::displacement(move_action.orientation);
      }
    }

    m_action_group.reset();
  }

  void MapScene::update([[maybe_unused]] gf::Time time)
  {
    m_orientation = gf::clamp(m_orientation, -1, +1);
    m_game->state()->hero.position += m_orientation;

    if (m_orientation != gf::vec(0, 0)) {
      m_game->state()->current_date += 15;
    }

    m_orientation = { 0,  0 };
  }

  void MapScene::render(gf::Console& console)
  {
    auto state = m_game->state();
    const gf::Vec2I hero_position = state->hero.position;
    const gf::RectI view = gf::RectI::from_center_size(hero_position, GameBoxSize);
    state->map.primary.blit_to(console, view, GameBoxPosition);

    gf::ConsoleStyle hero_style;
    hero_style.color.background = gf::Transparent;
    hero_style.color.foreground = gf::Black;
    hero_style.effect = gf::ConsoleEffect::none();

    console.put_character(state->hero.position - view.position(), '@', hero_style);
  }

}
