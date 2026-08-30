#include "WorldState.h"

#include <gf2/core/Streams.h>
#include <gf2/core/SerializationAdapter.h>
#include <gf2/core/SerializationContainer.h>
#include <gf2/core/SerializationOps.h>
#include <gf2/core/SerializationUtilities.h>

#include "ActorData.h"
#include "WorldData.h"

namespace fw {

  namespace {

    template<typename T>
    bool check_type(T& object)
    {
      return object.data && object.data->type() == object.type();
      // return object.data && object.data->element.type() == object.component.type();
    }

  }

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
    journal.entries.push_back({ current_date, std::move(message) });
  }

  void WorldState::bind(const WorldData& data)
  {
    for (ActorState& actor : actors) {
      actor.data.bind_from(data.actors);

      if (actor.component.type() == ActorType::Human) {
        HumanComponent& human_component = actor.component.from<ActorType::Human>();

        for (InventoryItemState& item : human_component.inventory.items) {
          item.data.bind_from(data.items);
        }

        if (human_component.weapon.data) {
          human_component.weapon.data.bind_from(data.items);
          assert(check_type(human_component.weapon));
        }

        if (human_component.projectile.data) {
          human_component.projectile.data.bind_from(data.items);
        }
      }

    }

    for (ItemState& item : items) {
      item.data.bind_from(data.items);
      assert(check_type(item));
    }
  }


}
