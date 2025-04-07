#ifndef FFW_DATA_LEXICON_H
#define FFW_DATA_LEXICON_H

#include <algorithm>
#include <type_traits>
#include <vector>

#include <gf2/core/Id.h>

#include "DataLabel.h"

namespace ffw {

  template<typename T>
  using DataLexicon = std::vector<T>;

  namespace details {

    template<typename T>
    struct DataLexiconCompare {
      static_assert(std::is_same_v<decltype(T::label), DataLabel>);

      bool operator()(const T& lhs, const T& rhs) {
        return lhs.label.id < rhs.label.id;
      }

      bool operator()(const T& lhs, gf::Id rhs) {
        return lhs.label.id < rhs;
      }

      bool operator()(gf::Id lhs, const T& rhs) {
        return lhs < rhs.label.id;
      }
    };

  }

  template<typename T>
  inline
  void data_lexicon_sort(DataLexicon<T>& lexicon) {
    std::sort(lexicon.begin(), lexicon.end(), details::DataLexiconCompare<T>{});
  }

  template<typename T>
  inline
  const T* data_lexicon_find(const DataLexicon<T>& dict, gf::Id id) {
    auto [ begin, end ] = std::equal_range(dict.begin(), dict.end(), id, details::DataLexiconCompare<T>{});

    if (begin == end) {
      return nullptr;
    }

    assert(end - begin == 1);
    return &*begin;
  }

}

#endif // FFW_DATA_LEXICON_H
