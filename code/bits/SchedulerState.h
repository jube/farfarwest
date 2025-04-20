#ifndef FFW_SCHEDULER_STATE_H
#define FFW_SCHEDULER_STATE_H

#include <queue>

#include <gf2/core/TaggedVariant.h>
#include <gf2/core/TypeTraits.h>
#include <gf2/core/Vec2.h>

#include "Date.h"

namespace ffw {

  enum class TaskType : uint8_t {
    Actor,
    Train,
  };

  struct Task {
    Date date;
    TaskType type;
    uint32_t index;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<Task, Archive>& task)
  {
    return ar | task.date | task.type | task.index;
  }

  bool operator<(const Task& lhs, const Task& rhs);

  struct SchedulerState {
    std::priority_queue<Task> queue;

    bool is_hero_turn() const
    {
      const Task& top = queue.top();
      return top.type == TaskType::Actor && top.index == 0;
    }

  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<SchedulerState, Archive>& state)
  {
    return ar | state.queue;
  }

}

#endif // FFW_SCHEDULER_STATE_H
