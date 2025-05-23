#include "WorldState.h"

#include <gf2/core/Streams.h>
#include <gf2/core/SerializationAdapter.h>
#include <gf2/core/SerializationContainer.h>
#include <gf2/core/SerializationOps.h>
#include <gf2/core/SerializationUtilities.h>

#include "WorldData.h"

namespace ffw {

  void WorldState::load_from_file(const std::filesystem::path& filename)
  {
    gf::FileInputStream file(filename);
    gf::CompressedInputStream compressed(&file);
    gf::Deserializer ar(&compressed);

    ar | *this;
  }

  void WorldState::save_to_file(const std::filesystem::path& filename) const
  {
    gf::FileOutputStream file(filename);
    gf::CompressedOutputStream compressed(&file);
    gf::Serializer ar(&compressed, StateVersion);

    ar | *this;
  }

  void WorldState::add_message(std::string message)
  {
    log.messages.push_back({ current_date, std::move(message) });
  }

  void WorldState::bind(const WorldData& data)
  {
    for (ActorState& actor : actors) {
      actor.data.bind_from(data.actors);

      for (InventoryItemState& item : actor.inventory.items) {
        item.data.bind_from(data.items);
      }

      if (actor.weapon.data) {
        actor.weapon.data.bind_from(data.items);
      }

      if (actor.ammunition.data) {
        actor.ammunition.data.bind_from(data.items);
      }
    }

    for (ItemState& item : items) {
      item.data.bind_from(data.items);
    }
  }


}
