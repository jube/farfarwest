#ifndef FFW_WORLD_DATA_H
#define FFW_WORLD_DATA_H

#include "ActorData.h"
#include "DataLexicon.h"

namespace ffw {

  struct WorldData {
    DataLexicon<ActorData> actors;

    void load_from_file(const std::filesystem::path& filename);

  };

}

#endif // FFW_WORLD_DATA_H
