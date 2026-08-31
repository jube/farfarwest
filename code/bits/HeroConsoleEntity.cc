#include "HeroConsoleEntity.h"

#include <cassert>

#include <gf2/core/ConsoleOperations.h>

#include "Colors.h"
#include "FarWest.h"
#include "ItemData.h"
#include "Settings.h"
#include "Styles.h"

namespace fw {

  namespace {

    std::string_view phase_style(Phase phase)
    {
      switch (phase) {
        case Phase::Night:
          return "nightlight";
        case Phase::Noon:
          return "noon";
        case Phase::Dawn:
        case Phase::Dusk:
          return "twilight";
        default:
          return "daylight";
      }
    }

    std::string_view phase_symbol(Phase phase) {
      return phase == Phase::Night ? "☾" : "☼";
    }

    std::string_view gender_style(Gender gender)
    {
      switch (gender) {
        case Gender::Girl:
          return "girl";
        case Gender::Boy:
          return "boy";
        case Gender::NonBinary:
          return "non_binary";
      }

      assert(false);
      return "unknown";
    }

    std::string_view gender_symbol(Gender gender)
    {
      switch (gender) {
        case Gender::Girl:
          return "♀";
        case Gender::Boy:
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

  HeroConsoleEntity::HeroConsoleEntity(FarWest* game)
  : m_game(game)
  {
  }

  void HeroConsoleEntity::update([[maybe_unused]] gf::Time time)
  {
  }

  void HeroConsoleEntity::render(gf::Console& console)
  {
    const WorldState* state = m_game->state();
    const WorldRuntime* runtime = m_game->runtime();

    gf::console_draw_frame(console, CharacterBox, gf::Gray);

    const gf::ConsoleRichStyle& rich_style = message_rich_style();

    gf::Vec2I position = CharacterBoxPosition + 1;

    const Date current_date = state->current_date;
    gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=date>{}</>", current_date.to_string());
    gf::console_print_picture(console, position + gf::dirx(12), gf::ConsoleAlignment::Left, rich_style, "<style={}>{}</>", phase_style(runtime->phase), phase_symbol(runtime->phase));

    position.y += 2;

    const ActorState& hero = state->hero();
    const HumanComponent& component = hero.component.from<ActorType::Human>();

    gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=hero>{}</>", component.name);
    ++position.y;
    gf::console_print_picture(console, position, gf::ConsoleAlignment::Left, rich_style, "<style={}>{}</>", gender_style(component.gender), gender_symbol(component.gender));
    gf::console_print_text(console, position + gf::dirx(1), gf::ConsoleAlignment::Left, rich_style, "{} year old", component.age);

    position.y += 2;

    gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=health>HP</>:");
    gf::console_print_picture(console, position + gf::dirx(2), gf::ConsoleAlignment::Left, rich_style, "<style=health>{}</><style=non_health>{}</>", health_bar(component.body.health), health_bar(MaxHealth - component.body.health));

    position.y += 2;

    auto print_attribute_stat = [&](std::string_view attribute_name, std::string_view attribute_style, int8_t attribute, std::string_view stat_name, gf::Color stat_color, const Stat& stat) {
      gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style={}>{}</>: {}", attribute_style, attribute_name, attribute);
      ++position.y;

      const int stat_bar = (stat * (CharacterBoxSize.w - 2) / 100).as_int();

      for (int x = 0; x < CharacterBoxSize.w - 2; ++x) {
        gf::console_write_background(console, position + gf::dirx(x), x <= stat_bar ? stat_color : gf::Gray);
      }

      gf::console_print_text(console, position, gf::ConsoleAlignment::Left, gf::Black, "{}: {}", stat_name, stat.as_int());
      ++position.y;
    };

    print_attribute_stat("FOR", "force", component.body.force, "Intensity", ForceColor, component.body.intensity);
    print_attribute_stat("DEX", "dexterity", component.body.dexterity, "Precision", DexterityColor, component.body.precision);
    print_attribute_stat("CON", "constitution", component.body.constitution, "Endurance", ConstitutionColor, component.body.endurance);

    ++position.y;

    gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=weapon>Weapon</>: {}", component.weapon.data ? component.weapon.data->label.tag : "-");
    ++position.y;

    if (component.projectile.data) {
      assert(component.projectile.data->element.type() == ItemType::Projectile);
      const ProjectileElement& projectile_element = component.projectile.data->element.from<ItemType::Projectile>();

      gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=weapon>Projectiles</>: {} × {}", component.projectile.data.tag, component.projectile.count); // only for distance weapons

      ++position.y;

      assert(component.weapon.data->element.type() == ItemType::DistanceWeapon);
      const DistanceWeaponElement& weapon_element = component.weapon.data->element.from<ItemType::DistanceWeapon>();
      const DistanceWeaponComponent& weapon_component = component.weapon.component.from<ItemType::DistanceWeapon>();

      gf::console_write_picture(console, position, u'║' /* u'[' */, gf::White);

      for (int16_t i = 0; i < weapon_component.projectiles; ++i) {
        gf::console_write_picture(console, position + gf::dirx(1 + i), component.projectile.data->display.picture, component.projectile.data->display.color);
      }

      for (int16_t i = weapon_component.projectiles; i < weapon_element.capacity; ++i) {
        gf::console_write_picture(console, position + gf::dirx(1 + i), projectile_element.empty.picture, projectile_element.empty.color);
      }

      gf::console_write_picture(console, position + gf::dirx(1 + weapon_element.capacity), u'║' /* u']' */, gf::White);


    } else {
      gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=weapon>Projectiles</>: -");
      ++position.y;
      gf::console_write_picture(console, position, "║║", rich_style.default_style());
    }

    position.y += 2;

    gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=cash>Cash</>: 100$");
    ++position.y;
    gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=debt>Debt</>: 10034$");
    ++position.y;


  }

}
