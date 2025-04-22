#ifndef FFW_MESSAGE_LOG_ELEMENT_H
#define FFW_MESSAGE_LOG_ELEMENT_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleElement.h>

namespace ffw {
  class FarFarWest;

  class MessageLogElement : public gf::ConsoleElement {
  public:
    MessageLogElement(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_MESSAGE_LOG_ELEMENT_H
