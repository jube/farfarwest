#include "FarFarWestScene.h"

#include "FarFarWestSystem.h"

namespace ffw {

  FarFarWestScene::FarFarWestScene(FarFarWestSystem* game, const FarFarWestResources& resources)
  : m_game(game)
  , m_action_group(compute_settings())
  , m_console_entity(resources.console_resource, game->resource_manager())
  {
    auto bounds = m_console_scene_manager.console().size() * 64;
    set_clear_color(gf::Black);
    set_world_size(bounds);
    set_world_center(bounds / 2);

    add_world_entity(&m_console_entity);
  }

  gf::ActionGroupSettings FarFarWestScene::compute_settings()
  {
    using namespace gf::literals;
    gf::ActionGroupSettings settings;

    settings.actions.emplace("fullscreen"_id, gf::instantaneous_action().add_keycode_control(gf::Keycode::F));

    return settings;
  }

  void FarFarWestScene::do_process_event(const gf::Event& event)
  {
    m_action_group.process_event(event);
    m_console_scene_manager.process_event(event);
  }

  void FarFarWestScene::do_handle_actions()
  {
    using namespace gf::literals;

    if (m_action_group.active("fullscreen"_id)) {
      m_game->window()->toggle_fullscreen();
    }

    m_console_scene_manager.handle_actions();

    m_action_group.reset();
  }

  void FarFarWestScene::do_update(gf::Time time)
  {
    m_console_scene_manager.update(time);
    m_console_scene_manager.render();
    m_console_entity.graphics().update(m_console_scene_manager.console(), m_game->render_manager());
    update_entities(time);
  }

}
