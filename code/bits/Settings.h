#ifndef FFW_SETTINGS_H
#define FFW_SETTINGS_H

#include <gf2/core/Rect.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  constexpr gf::Vec2I ConsoleSize = { 96, 54 };
  // constexpr gf::Vec2I ConsoleSize = { 80, 45 };

  constexpr gf::Vec2I GameBoxPosition = { 0, 0 };
  constexpr gf::Vec2I GameBoxSize = { 76, 49 };
  constexpr gf::RectI GameBox = gf::RectI::from_position_size(GameBoxPosition, GameBoxSize);

  constexpr gf::Vec2I MessageBoxPosition = { 0, 47 };
  constexpr gf::Vec2I MessageBoxSize = { 76, 7 };
  constexpr gf::RectI MessageBox = gf::RectI::from_position_size(MessageBoxPosition, MessageBoxSize);

  constexpr gf::Vec2I CharacterBoxPosition = { 76, 0 };
  constexpr gf::Vec2I CharacterBoxSize = { 20, 27 };
  constexpr gf::RectI CharacterBox = gf::RectI::from_position_size(CharacterBoxPosition, CharacterBoxSize);

  constexpr gf::Vec2I ContextualBoxPosition = { 76, 27 };
  constexpr gf::Vec2I ContextualBoxSize = { 20, 27 };
  constexpr gf::RectI ContextualBox = gf::RectI::from_position_size(ContextualBoxPosition, ContextualBoxSize);

}

#endif // FFW_SETTINGS_H
