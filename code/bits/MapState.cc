#include "MapState.h"

#include <gf2/core/FieldOfVision.h>

#include "ActorState.h"

namespace ffw {

  void clear_visible(BackgroundMap& map)
  {
    for (MapCell& cell : map) {
      cell.properties.reset(MapCellProperty::Visible);
    }
  }

  std::vector<gf::Vec2I> compute_hero_fov(gf::Vec2I position, BackgroundMap& state_map)
  {
    std::vector<gf::Vec2I> explored;

    clear_visible(state_map);

    gf::compute_symmetric_shadowcasting(state_map, state_map, position, HeroVisionRange, [&explored]([[maybe_unused]] gf::Vec2I position, MapCell& cell) {
      cell.properties.set(MapCellProperty::Visible);

      if (!cell.properties.test(MapCellProperty::Explored)) {
        explored.push_back(position);
        cell.properties.set(MapCellProperty::Explored);
      }
    });

    return explored;
  }

  BackgroundMap& MapState::from_floor(Floor floor)
  {
    switch (floor) {
      case Floor::Underground:
        return underground;
      case Floor::Ground:
        return ground;
      case Floor::Upstairs:
        return ground; // TODO: upstairs
    }

    assert(false);
    return ground;
  }

  const BackgroundMap& MapState::from_floor(Floor floor) const
  {
    switch (floor) {
      case Floor::Underground:
        return underground;
      case Floor::Ground:
        return ground;
      case Floor::Upstairs:
        return ground; // TODO: upstairs
    }

    assert(false);
    return ground;
  }

}
