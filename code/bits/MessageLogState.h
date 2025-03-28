#ifndef FFW_MESSAGE_LOG_STATE_H
#define FFW_MESSAGE_LOG_STATE_H

#include <string>
#include <vector>

#include <gf2/core/TypeTraits.h>

#include "Date.h"

namespace ffw {

  struct MessageState {
    Date date;
    std::string message;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MessageState, Archive>& state)
  {
    return ar | state.date | state.message;
  }

  struct MessageLogState {
    std::vector<MessageState> messages;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<MessageLogState, Archive>& state)
  {
    return ar | state.messages;
  }

}

#endif // FFW_MESSAGE_LOG_STATE_H
