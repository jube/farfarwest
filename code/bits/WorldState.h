#ifndef FFW_WORLD_STATE_H
#define FFW_WORLD_STATE_H

#include <cstdint>

#include <filesystem>

#include <gf2/core/TypeTraits.h>

#include "HeroState.h"

namespace ffw {

  constexpr std::uint16_t StateVersion = 1;

  struct WorldState {
    HeroState hero;


    void load_from_file(const std::filesystem::path& filename);
    void save_to_file(const std::filesystem::path& filename) const;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<WorldState, Archive>& state)
  {
    return ar | state.hero;
  }

}

#endif // FFW_WORLD_STATE_H
