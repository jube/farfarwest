#ifndef FFW_SETTINGS_H
#define FFW_SETTINGS_H

#include <cstdint>

#include <gf2/core/Rect.h>
#include <gf2/core/Vec2.h>

namespace ffw {

  constexpr int32_t WorldBasicSize = 4096;
  constexpr gf::Vec2I WorldSize = { WorldBasicSize, WorldBasicSize };
  constexpr gf::Vec2I WorldCenter = WorldSize / 2;

  constexpr gf::Vec2I ConsoleSize = { 96, 54 };
  // constexpr gf::Vec2I ConsoleSize = { 80, 45 };

  constexpr gf::Vec2I GameBoxPosition = { 0, 0 };
  constexpr gf::Vec2I GameBoxSize = { 72, 48 };
  constexpr gf::RectI GameBox = gf::RectI::from_position_size(GameBoxPosition, GameBoxSize);

  constexpr gf::Vec2I MessageBoxPosition = { 0, 48 };
  constexpr gf::Vec2I MessageBoxSize = { 72, 6 };
  constexpr gf::RectI MessageBox = gf::RectI::from_position_size(MessageBoxPosition, MessageBoxSize);

  constexpr gf::Vec2I CharacterBoxPosition = { 72, 0 };
  constexpr gf::Vec2I CharacterBoxSize = { 24, 27 };
  constexpr gf::RectI CharacterBox = gf::RectI::from_position_size(CharacterBoxPosition, CharacterBoxSize);

  constexpr gf::Vec2I ContextualBoxPosition = { 72, 27 };
  constexpr gf::Vec2I ContextualBoxSize = { 24, 27 };
  constexpr gf::RectI ContextualBox = gf::RectI::from_position_size(ContextualBoxPosition, ContextualBoxSize);

  constexpr int32_t MinimapFactor = 16;

  constexpr int8_t MaxHealth = 10;

}

#endif // FFW_SETTINGS_H
