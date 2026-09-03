#include "ItemPreviewConsoleEntity.h"

#include <gf2/core/ConsoleOperations.h>

#include "ActorData.h"
#include "ActorState.h"
#include "FarWest.h"
#include "ItemData.h"
#include "Settings.h"
#include "Styles.h"

namespace fw {

  namespace {

    constexpr gf::Vec2I ItemPreviewImagePosition = { (ItemPreviewConsoleSize.x - ItemImageSize) / 2, 5 };
    constexpr gf::Vec2I ItemPreviewNamePosition = { ItemPreviewConsoleSize.x / 2, 27 };
    constexpr gf::Vec2I ItemPreviewDescriptionPosition = { ItemPreviewImagePosition.x, 30 };
    constexpr int32_t ItemPreviewDescriptionWidth = 19;

  }

  ItemPreviewConsoleEntity::ItemPreviewConsoleEntity(FarWest* game)
  : m_game(game)
  {
  }

  void ItemPreviewConsoleEntity::set_item(const ItemData* data)
  {
    m_data = data;
  }

  void ItemPreviewConsoleEntity::update([[maybe_unused]] gf::Time time)
  {

  }

  void ItemPreviewConsoleEntity::render(gf::Console& console)
  {
    assert(console.size() == ItemPreviewConsoleSize);

    if (m_data == nullptr) {
      return;
    }

    const HumanComponent& hero_component = m_game->state()->hero().component.from<ActorType::Human>();

    const gf::ConsoleStyle& style = ui_default_style();
    const gf::ConsoleRichStyle& rich_style = ui_rich_style();

    gf::console_clear(console, style);
    gf::console_draw_frame(console, gf::RectI::from_size(ItemPreviewConsoleSize), style);

    gf::console_blit_to(m_data->image, console, ItemPreviewImagePosition);

    gf::console_print_text(console, ItemPreviewNamePosition, gf::ConsoleAlignment::Center, rich_style, "<style=item>{}</>", m_data->label.tag);

    gf::Vec2I position = ItemPreviewDescriptionPosition;

    enum class ItemComparison {
      Enabled,
      Disabled,
    };

    enum class ItemRanking {
      LowerIsBetter,
      HigherIsBetter,
    };

    auto basic_print_key_value = [&]<typename T>(std::string_view property, fmt::format_string<T> fmt, T&& value) {
      gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=property>{}</>:", property);
      gf::console_print_text(console, position + gf::dirx(ItemPreviewDescriptionWidth), gf::ConsoleAlignment::Right, rich_style, fmt, std::forward<T>(value));
    };

    auto print_key_value = [&]<typename T>(std::string_view property, fmt::format_string<T> fmt, T&& value) {
      basic_print_key_value(property, fmt, std::forward<T>(value));
      ++position.y;
    };

    auto print_key_value_comparison = [&]<typename T>(std::string_view property, fmt::format_string<T> fmt, T&& value, ItemComparison comparison, std::remove_cvref_t<T> reference_value, ItemRanking ranking = ItemRanking::HigherIsBetter) {
      basic_print_key_value(property, fmt, std::forward<T>(value));

      if (comparison == ItemComparison::Enabled) {
        const gf::Vec2I comparison_position = position + gf::dirx(ItemPreviewDescriptionWidth + 2);

        if (value == reference_value) {
          gf::console_print_text(console, comparison_position, gf::ConsoleAlignment::Left, rich_style, "(=)");
        } else {
          if constexpr (std::is_integral_v<std::remove_cvref_t<T>>) {
            const std::remove_cvref_t<T> difference = value - reference_value;

            if (difference > 0 && ranking == ItemRanking::HigherIsBetter) {
              gf::console_print_text(console, comparison_position, gf::ConsoleAlignment::Left, rich_style, "(<style=better>{:+d}</>)", difference);
            } else {
              gf::console_print_text(console, comparison_position, gf::ConsoleAlignment::Left, rich_style, "(<style=worse>{:+d}</>)", difference);
            }
          }
        }
      }

      ++position.y;
    };

    print_key_value("Type", "{}", to_string(m_data->type()));

    gf::console_print_text(console, position, gf::ConsoleAlignment::Left, rich_style, "<style=property>Display</>:");
    gf::console_write_picture(console, position + gf::dirx(ItemPreviewDescriptionWidth), m_data->display.picture, { m_data->display.color, gf::White });
    ++position.y;

    ++position.y;

    switch (m_data->type()) {
      case ItemType::None:
        break;
      case ItemType::Container:
      {
        const ContainerElement& element = m_data->element.from<ItemType::Container>();
        print_key_value("Capacity", "{}", element.capacity);
        break;
      }
      case ItemType::MeleeWeapon:
      {
        const MeleeWeaponElement& element = m_data->element.from<ItemType::MeleeWeapon>();
        MeleeWeaponElement reference = element;
        ItemComparison comparison = ItemComparison::Disabled;

        if (hero_component.weapon.type() == ItemType::MeleeWeapon) {
          reference = hero_component.weapon.data->element.from<ItemType::MeleeWeapon>();
          comparison = ItemComparison::Enabled;
        }

        print_key_value_comparison("Attack", "{}", element.attack.as_int(), comparison, reference.attack.as_int());
        print_key_value_comparison("Modifier", "{:+d}", element.modifier, comparison, reference.modifier);
        print_key_value_comparison("Use Time", "{}s", element.use_time, comparison, reference.use_time, ItemRanking::LowerIsBetter);
        break;
      }
      case ItemType::DistanceWeapon:
      {
        const DistanceWeaponElement& element = m_data->element.from<ItemType::DistanceWeapon>();
        DistanceWeaponElement reference = element;
        ItemComparison comparison = ItemComparison::Disabled;

        if (hero_component.weapon.type() == ItemType::DistanceWeapon) {
          reference = hero_component.weapon.data->element.from<ItemType::DistanceWeapon>();
          comparison = ItemComparison::Enabled;
        }

        print_key_value("Projectile", "{}", to_string(element.projectile));
        print_key_value_comparison("Capacity", "{}", element.capacity, comparison, reference.capacity);
        print_key_value_comparison("Range", "{}m", element.range, comparison, reference.range);
        print_key_value_comparison("Modifier", "{:+d}", element.modifier, comparison, reference.modifier);
        print_key_value_comparison("Shoot Time", "{}s", element.shoot_time, comparison, reference.shoot_time, ItemRanking::LowerIsBetter);
        print_key_value_comparison("Reload Time", "{}s", element.reload_time, comparison, reference.reload_time, ItemRanking::LowerIsBetter);
        break;
      }
      case ItemType::Projectile:
      {
        const ProjectileElement& element = m_data->element.from<ItemType::Projectile>();
        ProjectileElement reference = element;
        ItemComparison comparison = ItemComparison::Disabled;

        if (hero_component.projectile.data && hero_component.projectile.data->type() == ItemType::Projectile) {
          reference = hero_component.projectile.data->element.from<ItemType::Projectile>();
          comparison = ItemComparison::Enabled;
        }

        print_key_value("Kind", "{}", to_string(element.kind));
        print_key_value_comparison("Attack", "{}", element.attack.as_int(), comparison, reference.attack.as_int());
        print_key_value_comparison("Modifier", "{:+d}", element.modifier, comparison, reference.modifier);
        break;
      }
    }
  }


}
