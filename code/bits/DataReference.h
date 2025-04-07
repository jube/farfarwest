#ifndef FFW_DATA_REFERENCE_H
#define FFW_DATA_REFERENCE_H

#include <cassert>

#include <gf2/core/Id.h>
#include <gf2/core/Log.h>
#include <gf2/core/TypeTraits.h>

#include "DataLexicon.h"

namespace ffw {

  template<typename T>
  struct DataReference {
    std::string tag;
    gf::Id id = gf::InvalidId;
    const T* origin = nullptr;

    DataReference& operator=(std::string other) {
      tag = std::move(other);
      id = gf::hash_string(tag);
      return *this;
    }

    void bind_from(const DataLexicon<T>& lexicon)
    {
      origin = data_lexicon_find(lexicon, id);

      if (origin == nullptr) {
        gf::Log::error("Could not bind reference '{}' ({:x})", tag, static_cast<uint64_t>(id));
      }
    }

    explicit operator bool() const noexcept
    {
      return id != gf::InvalidId;
    }

    bool check() const noexcept
    {
      return origin != nullptr;
    }

    const T* operator->() const
    {
      assert(origin != nullptr);
      return origin;
    }

    const T& operator()() const
    {
      assert(origin != nullptr);
      return *origin;
    }

    void reset()
    {
      tag.clear();
      id = gf::InvalidId;
      origin = nullptr;
    }
  };

  namespace details {

    template<typename Archive, typename Self>
    Archive& handle_data_reference_serialization(Archive& ar, Self& self)
    {
      return ar | self.tag | self.id;
    }

  }

  template<typename Archive, typename T>
  Archive& operator|(Archive& ar, DataReference<T>& reference) {
    return details::handle_data_reference_serialization(ar, reference);
  }

  template<typename Archive, typename T>
  Archive& operator|(Archive& ar, const DataReference<T>& reference) {
    return details::handle_data_reference_serialization(ar, reference);
  }

}

#endif // FFW_DATA_REFERENCE_H
