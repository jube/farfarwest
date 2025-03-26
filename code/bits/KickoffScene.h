#ifndef FFW_KICKOFF_SCENE_H
#define FFW_KICKOFF_SCENE_H

#include <gf2/core/ConsoleScene.h>

namespace ffw {
  class FarFarWest;

  class KickoffScene : public gf::ConsoleScene {
  public:
    KickoffScene(FarFarWest* game);

    void render(gf::Console& buffer) override;

  private:
    FarFarWest* m_game = nullptr;
  };

}

#endif // FFW_KICKOFF_SCENE_H
