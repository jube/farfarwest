#include "MapState.h"

namespace ffw {

  void clear_visible(BackgroundMap& map)
  {
    for (MapCell& cell : map) {
      cell.properties.reset(MapCellProperty::Visible);
    }
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
