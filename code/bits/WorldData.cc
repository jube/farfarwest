#include "WorldData.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace ffw {

  void WorldData::load_from_file(const std::filesystem::path& filename)
  {
    std::ifstream ifs(filename);
    const nlohmann::json json = nlohmann::json::parse(ifs);

    json.at("actors").get_to(actors);
    data_lexicon_sort(actors);
  }

}
