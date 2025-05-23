#include "MessageLogElement.h"

#include <cstdint>

#include "FarFarWest.h"
#include "MessageLogState.h"
#include "Settings.h"
#include "WorldState.h"

namespace ffw {

  MessageLogElement::MessageLogElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void MessageLogElement::update([[maybe_unused]] gf::Time time)
  {
  }

  void MessageLogElement::render(gf::Console& console)
  {
    const WorldState* state = m_game->state();
    const std::vector<MessageState>& messages = state->log.messages;
    int32_t count = 0;
    std::string log;

    for (auto iterator = messages.rbegin(); iterator != messages.rend(); ++iterator) {
      log += fmt::format("<style=date>{}</>: {}\n", iterator->date.to_string_hours_minutes(), iterator->message);
      ++count;

      if (count == MessageBoxSize.h) {
        break;
      }
    }

    console.print_area(MessageBox, gf::ConsoleAlignment::Left, m_game->style(), log);
  }

}
