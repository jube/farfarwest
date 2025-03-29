#ifndef FFW_WORLD_STATE_H
#define FFW_WORLD_STATE_H

#include <cstdint>

#include <filesystem>

#include <gf2/core/TypeTraits.h>

#include "HeroState.h"
#include "MapState.h"
#include "MessageLogState.h"

namespace ffw {

  constexpr std::uint16_t StateVersion = 1;

  struct WorldState {
    MapState map;
    HeroState hero;


    MessageLogState log;

    void load_from_file(const std::filesystem::path& filename);
    void save_to_file(const std::filesystem::path& filename) const;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<WorldState, Archive>& state)
  {
    return ar | state.map | state.hero | state.log;
  }

}

#endif // FFW_WORLD_STATE_H
