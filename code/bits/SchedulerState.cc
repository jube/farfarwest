#include "SchedulerState.h"

namespace ffw {

  bool operator<(const Task& lhs, const Task& rhs)
  {
    return rhs.date < lhs.date; // lhs and rhs are reversed so that smaller dates appears first in the queue
  }

}
