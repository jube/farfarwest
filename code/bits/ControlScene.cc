#include "ControlScene.h"

#include "ActorState.h"
#include "FarFarWest.h"
#include "MapRuntime.h"
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

    settings.actions.emplace("minimap"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::Tab));

    settings.actions.emplace("mount"_id, gf::instantaneous_action().add_keycode_control(gf::Keycode::M));
    settings.actions.emplace("reload"_id, gf::instantaneous_action().add_keycode_control(gf::Keycode::R));

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

    if (m_action_group.active("idle"_id)) {
      runtime->hero.idle();
    }

    if (m_action_group.active("go"_id)) {
      if (runtime->hero.moves.empty()) {
        runtime->hero.moves = std::move(m_computed_path);
        m_mouse = std::nullopt;
      }
    }

    if (m_action_group.active("mount"_id)) {
      if (state->hero().feature.from<ActorType::Human>().mounting == NoIndex) {
        runtime->hero.mount();
      } else {
        runtime->hero.dismount();
      }
    }

    if (m_action_group.active("reload"_id)) {
      runtime->hero.reload();
    }

    if (m_action_group.active("minimap"_id)) {
      m_game->pop_all_scenes();
      m_game->push_scene(&m_game->minimap);
    }

    if (m_action_group.active("escape"_id)) {
      m_game->replace_scene(&m_game->quit);
    }

    m_action_group.reset();
  }

  void ControlScene::update([[maybe_unused]] gf::Time time)
  {
    if (!m_mouse) {
      m_computed_path.clear();
      return;
    }

    update_grid();

    const WorldState* state = m_game->state();
    WorldRuntime* runtime = m_game->runtime();

    const gf::Vec2I target = *m_mouse + runtime->compute_view().position();

    if (!m_computed_path.empty() && m_computed_path.front() == target) {
      return;
    }

    if (runtime->map.ground.reverse(target).empty() && runtime->map.ground.grid.walkable(target)) {

      if (runtime->hero.moves.empty()) {
        gf::Log::debug("computing path to {},{}", target.x, target.y);
        m_computed_path = m_grid.compute_route(state->hero().position, target);
        gf::Log::debug("path computed");
      }

      if (!m_computed_path.empty()) {
        std::reverse(m_computed_path.begin(), m_computed_path.end());
        m_computed_path.pop_back();
      }
    }
  }

  void ControlScene::render(gf::Console& console)
  {
    const WorldRuntime* runtime = m_game->runtime();
    const gf::RectI view = runtime->compute_view();

    const std::vector<gf::Vec2I>& path = !runtime->hero.moves.empty() ? runtime->hero.moves : m_computed_path;

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

  void ControlScene::update_grid()
  {
    const WorldState* state = m_game->state();

    if (state->current_date == m_last_grid_update) {
      return;
    }

    gf::Log::debug("update grid");

    const WorldRuntime* runtime = m_game->runtime();

    m_grid = runtime->map.ground.grid;

    for (const ActorState& actor : state->actors) {
      m_grid.set_walkable(actor.position, false);
    }

    for (const TrainState& train : state->network.trains) {
      uint32_t offset = 0;

      for (uint32_t k = 0; k < TrainLength; ++k) {
        const uint32_t index = runtime->network.next_position(train.railway_index, offset);
        assert(index < runtime->network.railway.size());
        const gf::Vec2I position = runtime->network.railway[index];


        for (int32_t i = -1; i <= 1; ++i) {
          for (int32_t j = -1; j <= 1; ++j) {
            const gf::Vec2I neighbor = { i, j };
            const gf::Vec2I neighbor_position = position + neighbor;

            m_grid.set_walkable(neighbor_position, false);
          }
        }

        offset += 3;
      }
    }

    const gf::RectI view = runtime->compute_view();
    const gf::Vec2I view_nw = view.position_at(gf::Orientation::NorthWest) - 1;
    const gf::Vec2I view_se = view.position_at(gf::Orientation::SouthEast) + 1;

    for (int x = view_nw.x; x <= view_se.x; ++x) {
      m_grid.set_walkable({ x, view_nw.y }, false);
      m_grid.set_walkable({ x, view_se.y }, false);
    }

    for (int y = view_nw.y; y <= view_se.y; ++y) {
      m_grid.set_walkable({ view_nw.x, y }, false);
      m_grid.set_walkable({ view_se.x, y }, false);
    }

    m_last_grid_update = state->current_date;
    m_computed_path.clear();
  }

}
