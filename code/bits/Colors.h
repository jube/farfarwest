#ifndef FFW_COLORS_H
#define FFW_COLORS_H

#include <gf2/core/Color.h>

namespace ffw {

  inline constexpr gf::Color RpgBlue = { 0.015625f, 0.03125f, 0.515625f };

  inline constexpr gf::Color PrairieColor = 0xC4D6B0;
  inline constexpr gf::Color DesertColor = 0xC2B280;
  inline constexpr gf::Color ForestColor = 0x4A6A4D;
  inline constexpr gf::Color MountainColor = 0x8B5A2B;

  inline constexpr gf::Color RockColor = gf::gray(0.25f);
  inline constexpr gf::Color DirtColor = 0xB69F66;

  inline constexpr gf::Color StreetColor = 0xECBD6B;

  inline constexpr gf::Color ForceColor = gf::Amber;
  inline constexpr gf::Color DexterityColor = gf::Aquamarine;
  inline constexpr gf::Color ConstitutionColor = gf::Purple;

}

#endif // FFW_COLORS_H
