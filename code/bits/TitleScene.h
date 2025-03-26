#ifndef FFW_TITLE_SCENE_H
#define FFW_TITLE_SCENE_H

#include <gf2/core/Console.h>
#include <gf2/core/ConsoleScene.h>
#include <gf2/core/FontFace.h>

namespace ffw {
  class FarFarWest;

  class TitleScene : public gf::ConsoleScene {
  public:
    TitleScene(FarFarWest* game);

    void render(gf::Console& buffer) override;

  private:
    FarFarWest* m_game = nullptr;
    gf::Console m_title;
  };

}

#endif // FFW_TITLE_SCENE_H
