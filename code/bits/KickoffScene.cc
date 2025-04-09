#include "KickoffScene.h"

#include "FarFarWest.h"

namespace ffw {

  namespace {
    constexpr int ChoiceCount = 3;
    constexpr int StartNewGameChoice = 0;
    constexpr int ContinueGameChoice = 1;
    constexpr int QuitChoice = 2;

  }

  KickoffScene::KickoffScene(FarFarWest* game)
  : m_game(game)
  , m_action_group(compute_settings())
  {
  }

  gf::ActionGroupSettings KickoffScene::compute_settings()
  {
    using namespace gf::literals;
    gf::ActionGroupSettings settings;

    settings.actions.emplace("down"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::Down));
    settings.actions.emplace("up"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::Up));
    settings.actions.emplace("choose"_id, gf::instantaneous_action().add_scancode_control(gf::Scancode::Space));

    return settings;
  }

  void KickoffScene::process_event(const gf::Event& event)
  {
    m_action_group.process_event(event);
  }

  void KickoffScene::handle_actions()
  {
    using namespace gf::literals;

    if (m_action_group.active("down"_id)) {
      m_choice = (m_choice + 1) % ChoiceCount;
    } else if (m_action_group.active("up"_id)) {
      m_choice = (m_choice + ChoiceCount- 1) % ChoiceCount;
    } else if (m_action_group.active("choose"_id)) {
      switch (m_choice) {
        case StartNewGameChoice:
          m_game->start_world_generation(AdventureChoice::New);
          m_game->replace_scene(&m_game->generation);
          break;
        case ContinueGameChoice:
          if (m_game->has_save()) {
            m_game->start_world_generation(AdventureChoice::Saved);
            m_game->replace_scene(&m_game->generation);
          }
          break;
        case QuitChoice:
          m_game->pop_all_scenes();
          break;
      }
    }

    m_action_group.reset();
  }

  void KickoffScene::render(gf::Console& console)
  {
#if 0
    gf::ConsoleStyle style;
    style.color.background = gf::Black;
    style.color.foreground = gf::Red;

    console.draw_frame(MessageBox, style);

    style.color.foreground = gf::Chartreuse;

    console.draw_frame(CharacterBox, style);

    style.color.foreground = gf::Orange;

    console.draw_frame(ContextualBox, style);
#endif

    gf::ConsoleStyle style;

    style.color.foreground = gf::White;
    console.print({ 35, 35 }, gf::ConsoleAlignment::Left, style, "Start a new adventure");

    if (m_game->has_save()) {
      style.color.foreground = gf::White;
    } else {
      style.color.foreground = gf::Gray;
    }

    console.print({ 35, 36 }, gf::ConsoleAlignment::Left, style, "Continue the previous adventure");

    style.color.foreground = gf::White;
    console.print({ 35, 37 }, gf::ConsoleAlignment::Left, style, "Quit");

    style.color.foreground = gf::White;
    console.print({ 33, 35 + m_choice }, gf::ConsoleAlignment::Left, style, ">");
  }

}
