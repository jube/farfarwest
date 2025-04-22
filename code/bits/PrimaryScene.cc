#include "PrimaryScene.h"

#include "FarFarWest.h"

namespace ffw {

  PrimaryScene::PrimaryScene(FarFarWest* game)
  : m_game(game)
  , m_message_log_element(game)
  , m_map_element(game)
  , m_hero_element(game)
  , m_contextual_element(game)
  {
    add_model(game->model());

    add_element(&m_message_log_element);
    add_element(&m_map_element);
    add_element(&m_hero_element);
    add_element(&m_contextual_element);
  }

}
