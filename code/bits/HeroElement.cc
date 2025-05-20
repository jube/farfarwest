#include "HeroElement.h"

#include <cassert>

#include "Colors.h"
#include "FarFarWest.h"
#include "Settings.h"
#include "gf2/core/Color.h"
#include "gf2/core/ConsoleEffect.h"
#include "gf2/core/ConsoleStyle.h"

namespace ffw {

  namespace {

    std::string_view gender_style(Gender gender)
    {
      switch (gender) {
        case Gender::Female:
          return "female";
        case Gender::Male:
          return "male";
        case Gender::NonBinary:
          return "non_binary";
      }

      assert(false);
      return "unknown";
    }

    std::string_view gender_symbol(Gender gender)
    {
      switch (gender) {
        case Gender::Female:
          return "♀";
        case Gender::Male:
          return "♂";
        case Gender::NonBinary:
          return "○";
      }

      assert(false);
      return "#";
    }

    std::string health_bar(int8_t health)
    {
      std::string health_string;

      for (int8_t i = 0; i < health; ++i) {
        health_string += "♥";
      }

      return health_string;
    }

  }

  HeroElement::HeroElement(FarFarWest* game)
  : m_game(game)
  {
  }

  void HeroElement::update([[maybe_unused]] gf::Time time)
  {
  }

  void HeroElement::render(gf::Console& console)
  {
    auto* state = m_game->state();

    gf::ConsoleStyle character_box_style;
    character_box_style.color.foreground = gf::Gray;
    console.draw_frame(CharacterBox, character_box_style);

    gf::Vec2I position = CharacterBoxPosition + 1;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), " <style=date>{}</>", state->current_date.to_string());
    position.y += 2;

    const ActorState& hero = state->hero();
    const HumanFeature& feature = hero.feature.from<ActorType::Human>();

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=hero>{}</>", feature.name);
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "{} year old, <style={}>{}</>", feature.age, gender_style(feature.gender), gender_symbol(feature.gender));

    position.y += 2;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=health>{}</><style=non_health>{}</>", health_bar(feature.health), health_bar(MaxHealth - feature.health));

    position.y += 2;

    gf::ConsoleStyle stat_style;
    stat_style.effect = gf::ConsoleEffect::add();
    stat_style.color.foreground = gf::Black;
    stat_style.color.background = gf::Transparent;

    auto print_attribute_stat = [&](std::string_view attribute_name, std::string_view attribute_style, int8_t attribute, std::string_view stat_name, gf::Color stat_color, const Stat& stat) {
      console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style={}>{}</>: {}", attribute_style, attribute_name, attribute);
      ++position.y;

      const int stat_bar = (stat * (CharacterBoxSize.w - 2) / 100).as_int();

      for (int x = 0; x < CharacterBoxSize.w - 2; ++x) {
        console.set_background(position + gf::dirx(x), x <= stat_bar ? stat_color : gf::Gray);
      }

      console.print(position, gf::ConsoleAlignment::Left, stat_style, "{}: {}", stat_name, stat.as_int());
      ++position.y;
    };

    print_attribute_stat("FOR", "force", feature.force, "Intensity", ForceColor, feature.intensity);
    print_attribute_stat("DEX", "dexterity", feature.dexterity, "Precision", DexterityColor, feature.precision);
    print_attribute_stat("CON", "constitution", feature.constitution, "Endurance", ConstitutionColor, feature.endurance);

    ++position.y;

#if 0
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), " <style=force>FOR</>:{:2} <style=dexterity>DEX</>:{:2} <style=constitution>CON</>:{:2} ", feature.force, feature.dexterity, feature.constitution);
    ++position.y;

    const int intensity_bar = (feature.intensity * (CharacterBoxSize.w - 2) / 100).as_int();
    const int precision_bar = (feature.precision * (CharacterBoxSize.w - 2) / 100).as_int();
    const int endurance_bar = (feature.endurance * (CharacterBoxSize.w - 2) / 100).as_int();

    for (int x = 0; x < CharacterBoxSize.w - 2; ++x) {
      console.set_background({ position.x + x, position.y + 0 }, x <= intensity_bar ? ForceColor : gf::Gray);
      console.set_background({ position.x + x, position.y + 1 }, x <= precision_bar ? DexterityColor : gf::Gray);
      console.set_background({ position.x + x, position.y + 2 }, x <= endurance_bar ? ConstitutionColor : gf::Gray);
    }

    gf::ConsoleStyle stat_style;
    stat_style.effect = gf::ConsoleEffect::add();
    stat_style.color.foreground = gf::Black;
    stat_style.color.background = gf::Transparent;

    console.print(position, gf::ConsoleAlignment::Left, stat_style, "Intensity: {}", feature.intensity.as_int());
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, stat_style, "Precision: {}", feature.precision.as_int());
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, stat_style, "Endurance: {}", feature.endurance.as_int());
#endif

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=weapon>Weapon</>:"); // put the type of the weapon here?
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "Colt Dragoon Revolver");
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=weapon>Ammunitions</>: .44"); // only for firearms
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "•••○○○ [32]");

    position.y += 2;

    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=cash>Cash</>: 100$");
    ++position.y;
    console.print(position, gf::ConsoleAlignment::Left, m_game->style(), "<style=debt>Debt</>: 10034$");
    ++position.y;


  }

}
