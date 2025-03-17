#ifndef FFW_HERO_STATE_H
#define FFW_HERO_STATE_H

#include <gf2/core/TypeTraits.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  struct HeroState {
    gf::Vec2I position;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<HeroState, Archive>& state)
  {
    return ar | state.position;
  }

}

#endif // FFW_HERO_STATE_H
