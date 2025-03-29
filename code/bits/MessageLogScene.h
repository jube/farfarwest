#ifndef FFW_MESSAGE_LOG_SCENE_H
#define FFW_MESSAGE_LOG_SCENE_H

#include <gf2/core/Time.h>
#include <gf2/core/ConsoleScene.h>

namespace ffw {
  class FarFarWest;

  class MessageLogScene : public gf::ConsoleScene {
  public:
    MessageLogScene(FarFarWest* game);

    void update(gf::Time time) override;
    void render(gf::Console& console) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_MESSAGE_LOG_SCENE_H
