#ifndef FFW_INVENTORY_STATE_H
#define FFW_INVENTORY_STATE_H

#include <cstdint>

#include <vector>

#include "ItemState.h"

namespace ffw {

  struct InventoryState {
    int32_t cash = 0;
    std::vector<ItemState> items;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<InventoryState, Archive>& state)
  {
    return ar | state.cash | state.items;
  }

}

#endif // FFW_INVENTORY_STATE_H
