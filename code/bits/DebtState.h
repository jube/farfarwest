#ifndef FFW_DEBT_STATE_H
#define FFW_DEBT_STATE_H

#include <vector>

#include "Date.h"

namespace ffw {

  struct Installment {
    int32_t amount;
    Date due_date;
    // TODO: lender
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<Installment, Archive>& installment)
  {
    return ar | installment.amount | installment.due_date;
  }

  struct DebtState {
    std::vector<Installment> installments;
  };

  template<typename Archive>
  Archive& operator|(Archive& ar, gf::MaybeConst<DebtState, Archive>& state)
  {
    return ar | state.installments;
  }

}

#endif // FFW_DEBT_STATE_H
