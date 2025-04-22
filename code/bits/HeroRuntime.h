#ifndef FFW_HERO_RUNTIME_H
#define FFW_HERO_RUNTIME_H

#include <vector>

#include <gf2/core/TaggedVariant.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  enum class ActionType {
    None,
    Move,
  };

  struct MoveAction {
    gf::Vec2I orientation = { 0, 0 };
  };

  using HeroAction = gf::TaggedVariant<ActionType, MoveAction>;


  struct HeroRuntime {
    HeroAction action;
    std::vector<gf::Vec2I> moves;

    void move(gf::Vec2I orientation)
    {
      action = MoveAction{ orientation };
    }

  };

}

#endif // FFW_HERO_RUNTIME_H
