#include "AdventureInventoryScene.h"

#include <gf2/core/ConsoleOperations.h>

#include "FarWest.h"
#include "WorldHelpers.h"

namespace fw {

  namespace {

  }

  AdventureInventoryScene::AdventureInventoryScene(FarWest* game)
  : m_game(game)
  , m_action_group(compute_settings())
  , m_inventory(game)
  {
    add_entity(&m_inventory);
  }

  void AdventureInventoryScene::process_event(const gf::Event& event)
  {
    m_action_group.process_event(event);
  }

  void AdventureInventoryScene::handle_actions()
  {
    using namespace gf::literals;

    if (m_action_group.active("back"_id)) {
      m_game->replace_scene(&m_game->adventure_control);
    }

    if (m_action_group.active("prev_item"_id)) {
      m_inventory.prev_item();
    }

    if (m_action_group.active("next_item"_id)) {
      m_inventory.next_item();
    }

    if (m_action_group.active("prev_page"_id)) {
      m_inventory.prev_page();
    }

    if (m_action_group.active("next_page"_id)) {
      m_inventory.next_page();
    }

    if (m_action_group.active("equip"_id)) {
      std::optional<int32_t> maybe_index = m_inventory.current_index();

      if (maybe_index.has_value()) {
        equip_hero(*m_game->model(), maybe_index.value());
      }
    }

    m_action_group.reset();
  }

  void AdventureInventoryScene::update(gf::Time time)
  {
    update_inventory();
    update_entities(time);
  }

  void AdventureInventoryScene::render(gf::Console& console)
  {
    update_inventory();
    render_entities(console);
  }

  gf::ActionGroupSettings AdventureInventoryScene::compute_settings()
  {
    using namespace gf::literals;
    gf::ActionGroupSettings settings;

    settings.actions.emplace("back"_id, gf::instantaneous_action().add_keycode_control(gf::Keycode::I));
    settings.actions.emplace("prev_item"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::Up));
    settings.actions.emplace("next_item"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::Down));
    settings.actions.emplace("prev_page"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::PageUp));
    settings.actions.emplace("next_page"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::PageDown));

    settings.actions.emplace("equip"_id, gf::instantaneous_action().add_keycode_control(gf::Keycode::E));

    return settings;
  }

  void AdventureInventoryScene::update_inventory()
  {
    m_inventory.set_inventory(&m_game->state()->hero().component.from<ActorType::Human>().inventory);
  }

}
