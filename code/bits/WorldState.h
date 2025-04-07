#ifndef FFW_WORLD_STATE_H
#define FFW_WORLD_STATE_H

#include <cassert>
#include <cstdint>

#include <filesystem>

#include <gf2/core/TypeTraits.h>

#include "ActorState.h"
#include "Date.h"
#include "MapState.h"
#include "MessageLogState.h"

namespace ffw {
  struct WorldData;

  constexpr std::uint16_t StateVersion = 1;

  struct WorldState {
    Date current_date;

    MapState map;
    std::vector<ActorState> actors;

    MessageLogState log;

    ActorState& hero() {
      assert(!actors.empty());
      return actors.front();
    }

    const ActorState& hero() const {
      assert(!actors.empty());
      return actors.front();
    }

    void load_from_file(const std::filesystem::path& filename);
    void save_to_file(const std::filesystem::path& filename) const;

    void bind(const WorldData& data);
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<WorldState, Archive>& state)
  {
    return ar | state.map | state.actors | state.log;
  }

}

#endif // FFW_WORLD_STATE_H
