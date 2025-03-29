#include "MessageLogScene.h"

#include "FarFarWest.h"
#include "Settings.h"

namespace ffw {

  MessageLogScene::MessageLogScene(FarFarWest* game)
  : m_game(game)
  {
  }

  void MessageLogScene::update([[maybe_unused]] gf::Time time)
  {
  }

  void MessageLogScene::render(gf::Console& console)
  {
    auto* state = m_game->state();
    auto& last = state->log.messages.back();
    auto entry = fmt::format("<style=date>{}</>: {}", last.date.to_string(), last.message);
    console.print(MessageBoxPosition, gf::ConsoleAlignment::Left, m_game->style(), entry);

  }

}
