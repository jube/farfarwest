#include "ControlScene.h"

#include <gf2/core/Log.h>
#include <gf2/core/Orientation.h>

#include "FarFarWest.h"

namespace ffw {

  namespace {
    using namespace gf::literals;

    struct MoveAction {
      gf::Id id;
      gf::Orientation orientation;
      gf::Scancode key;
    };

    constexpr MoveAction MoveActions[] {
      { "go_south_west"_id, gf::Orientation::SouthWest, gf::Scancode::Numpad1 },
      { "go_south"_id,      gf::Orientation::South,     gf::Scancode::Numpad2 },
      { "go_south_east"_id, gf::Orientation::SouthEast, gf::Scancode::Numpad3 },
      { "go_west"_id,       gf::Orientation::West,      gf::Scancode::Numpad4 },
      { "idle"_id,          gf::Orientation::Center,    gf::Scancode::Numpad5 },
      { "go_east"_id,       gf::Orientation::East,      gf::Scancode::Numpad6 },
      { "go_north_west"_id, gf::Orientation::NorthWest, gf::Scancode::Numpad7 },
      { "go_north"_id,      gf::Orientation::North,     gf::Scancode::Numpad8 },
      { "go_north_east"_id, gf::Orientation::NorthEast, gf::Scancode::Numpad9 },
    };

  }

  ControlScene::ControlScene(FarFarWest* game)
  : m_game(game)
  , m_action_group(compute_settings())
  {
  }

  void ControlScene::process_event(const gf::Event& event)
  {
    m_action_group.process_event(event);
  }

  void ControlScene::handle_actions()
  {
    using namespace gf::literals;

    for (auto move_action : MoveActions) {
      if (m_action_group.active(move_action.id)) {
        m_orientation += gf::displacement(move_action.orientation);
      }
    }

    m_action_group.reset();
  }

  void ControlScene::update([[maybe_unused]] gf::Time time)
  {
    m_orientation = gf::clamp(m_orientation, -1, +1);
    m_game->state()->hero.position += m_orientation;
    m_orientation = { 0,  0 };
  }

  gf::ActionGroupSettings ControlScene::compute_settings()
  {
    using namespace gf::literals;
    gf::ActionGroupSettings settings;

    for (auto move_action : MoveActions) {
      settings.actions.emplace(move_action.id, gf::instantaneous_action().add_scancode_control(move_action.key));
    }

    return settings;
  }

}
