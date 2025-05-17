#ifndef FFW_HERO_RUNTIME_H
#define FFW_HERO_RUNTIME_H

#include <vector>

#include <gf2/core/TaggedVariant.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  enum class ActionType {
    None,
    Idle,
    Move,
  };

  struct IdleAction {
  };

  struct MoveAction {
    gf::Vec2I orientation = { 0, 0 };
  };

  using HeroAction = gf::TaggedVariant<ActionType, IdleAction, MoveAction>;


  struct HeroRuntime {
    HeroAction action;
    std::vector<gf::Vec2I> moves;

    void idle()
    {
      action = IdleAction{};
    }

    void move(gf::Vec2I orientation)
    {
      action = MoveAction{ orientation };
    }

  };

}

#endif // FFW_HERO_RUNTIME_H
