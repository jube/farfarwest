#include "MinimapScene.h"

#include "FarFarWest.h"
#include "gf2/core/Scancode.h"

namespace ffw {

  MinimapScene::MinimapScene(FarFarWest* game)
  : m_game(game)
  , m_action_group(compute_settings())
  , m_minimap(game)
  {
    add_element(&m_minimap);
  }

  void MinimapScene::process_event(const gf::Event& event)
  {
    m_action_group.process_event(event);
  }

  void MinimapScene::handle_actions()
  {
    using namespace gf::literals;

    if (m_action_group.active("zoom_in"_id)) {
      m_minimap.zoom_in();
    }

    if (m_action_group.active("zoom_out"_id)) {
      m_minimap.zoom_out();
    }

    if (m_action_group.active("back"_id)) {
      m_game->start_world();
    }

    m_action_group.reset();
  }

  gf::ActionGroupSettings MinimapScene::compute_settings()
  {
    using namespace gf::literals;
    gf::ActionGroupSettings settings;

    settings.actions.emplace("zoom_in"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::NumpadPlus).add_scancode_control(gf::Scancode::F11));
    settings.actions.emplace("zoom_out"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::NumpadMinus).add_scancode_control(gf::Scancode::F12));
    settings.actions.emplace("back"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::Tab));

    return settings;
  }

}
