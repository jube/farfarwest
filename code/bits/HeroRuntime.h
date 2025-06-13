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
    Mount,
    Reload,
  };

  struct IdleAction {
  };

  struct MoveAction {
    gf::Vec2I orientation = { 0, 0 };
  };

  struct MountAction {
  };

  struct ReloadAction {
  };

  using HeroAction = gf::TaggedVariant<ActionType, IdleAction, MoveAction, MountAction, ReloadAction>;


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

    void mount()
    {
      action = MountAction{};
    }

    void reload()
    {
      action = ReloadAction{};
    }

  };

}

#endif // FFW_HERO_RUNTIME_H
