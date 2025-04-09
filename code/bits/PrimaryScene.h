#ifndef FFW_PRIMARY_SCENE_H
#define FFW_PRIMARY_SCENE_H

#include <gf2/core/ConsoleScene.h>

#include "HeroElement.h"
#include "MapElement.h"
#include "MessageLogElement.h"

namespace ffw {
  class FarFarWest;

  class PrimaryScene : public gf::ConsoleScene {
  public:
    PrimaryScene(FarFarWest* game);

  private:
    FarFarWest* m_game = nullptr;

    MessageLogElement m_message_log_element;
    MapElement m_map_element;
    HeroElement m_hero_element;
  };


}

#endif // FFW_PRIMARY_SCENE_H
