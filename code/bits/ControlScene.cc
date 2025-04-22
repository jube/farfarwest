#include "ControlScene.h"

#include "ActorState.h"
#include "FarFarWest.h"
#include "MapRuntime.h"
#include "MapState.h"
#include "Settings.h"
#include "WorldRuntime.h"
#include "WorldState.h"

namespace ffw {

  namespace {
    using namespace gf::literals;

    struct IdMoveAction {
      gf::Id id;
      gf::Orientation orientation;
      gf::Scancode key;
      gf::Scancode alt_key;
    };

    constexpr IdMoveAction MoveActions[] {
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

  ControlScene::ControlScene(FarFarWest* game)
  : m_game(game)
  , m_action_group(compute_settings())
  {
  }

  void ControlScene::process_event(const gf::Event& event)
  {
    m_action_group.process_event(event);

    if (event.type() == gf::EventType::MouseMoved) {
      const gf::MouseMovedEvent mouse_moved_event = event.from<gf::EventType::MouseMoved>();
      const gf::Vec2I position = m_game->point_to(mouse_moved_event.position);
      if (GameBox.contains(position)) {
        m_mouse = position;
      } else {
        m_mouse = std::nullopt;
      }
    }
  }

  gf::ActionGroupSettings ControlScene::compute_settings()
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

    settings.actions.emplace("escape"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::Escape));
    settings.actions.emplace("go"_id, gf::instantaneous_action().add_mouse_button_control(gf::MouseButton::Left));

    return settings;
  }

  void ControlScene::handle_actions()
  {
    const WorldState* state = m_game->state();

    if (!state->scheduler.is_hero_turn()) {
      m_action_group.reset();
      return;
    }

    using namespace gf::literals;

    WorldRuntime* runtime = m_game->runtime();
    runtime->hero.action = {};

    gf::Vec2I orientation = { 0, 0 };

    for (auto move_action : MoveActions) {
      if (m_action_group.active(move_action.id)) {
        orientation += gf::displacement(move_action.orientation);
      }
    }

    if (orientation != gf::vec(0, 0)) {
      runtime->hero.move(orientation);
    }

    if (m_action_group.active("go"_id)) {
      if (runtime->hero.moves.empty()) {
        runtime->hero.moves = compute_path();
        m_mouse = std::nullopt;
      }
    }

    if (m_action_group.active("escape"_id)) {
      m_game->replace_scene(&m_game->quit);
    }

    m_action_group.reset();
  }

  void ControlScene::render(gf::Console& console)
  {
    const WorldRuntime* runtime = m_game->runtime();
    const gf::RectI view = runtime->compute_view();

    std::vector<gf::Vec2I> path = runtime->hero.moves;

    if (path.empty()) {
      path = compute_path();
    }

    gf::ConsoleStyle style;
    style.color.foreground = gf::gray(0.2f);
    style.effect = gf::ConsoleEffect::none();

    for (const gf::Vec2I position : path) {
      console.put_character(position - view.position(), u'·', style);
    }

    if (m_mouse) {
      console.put_character(*m_mouse, '+', style);
    }
  }

  std::vector<gf::Vec2I> ControlScene::compute_path()
  {
    std::vector<gf::Vec2I> path;

    if (m_mouse) {
      const WorldState* state = m_game->state();
      WorldRuntime* runtime = m_game->runtime();

      update_grid();

      const gf::Vec2I target = *m_mouse + runtime->compute_view().position();

      if (runtime->map.outside_reverse(target).empty() && runtime->map.outside_grid.walkable(target)) {

        if (runtime->hero.moves.empty()) {
          // gf::Log::debug("computing path to {},{}", target.x, target.y);
          path = m_grid.compute_route(state->hero().position, target);
        }

        if (!path.empty()) {
          std::reverse(path.begin(), path.end());
          path.pop_back();
        }
      }
    }

    return path;
  }

  void ControlScene::update_grid()
  {
    const WorldState* state = m_game->state();

    if (state->current_date == m_last_grid_update) {
      return;
    }

    // gf::Log::debug("updating grid");

    const WorldRuntime* runtime = m_game->runtime();

    m_grid = runtime->map.outside_grid;

    for (const ActorState& actor : state->actors) {
      m_grid.set_walkable(actor.position, false);
    }

    for (const TrainState& train : state->map.network.trains) {
      for (uint32_t i = 0; i < TrainSize; ++i) {
        const uint32_t index = state->map.network.next_position(train.index, i);
        assert(index < state->map.network.railway.size());
        const gf::Vec2I position = state->map.network.railway[index];
        m_grid.set_walkable(position, false);
      }
    }

    m_last_grid_update = state->current_date;
  }

}
