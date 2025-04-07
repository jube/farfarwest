#ifndef FFW_ACTOR_H
#define FFW_ACTOR_H

#include <gf2/core/Color.h>
#include <gf2/core/Vec2.h>

#include "ActorData.h"
#include "DataReference.h"

namespace ffw {

  struct ActorState {
    DataReference<ActorData> data;
    gf::Vec2I position;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<ActorState, Archive>& state)
  {
    return ar | state.data | state.position;
  }

}

#endif // FFW_ACTOR_H
