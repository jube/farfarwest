#include "Styles.h"

#include "Colors.h"

namespace fw {

  /*
   * Message styles (black background)
   */

  const gf::ConsoleStyle& message_default_style()
  {
    static constexpr gf::ConsoleStyle DefaultStyle = { gf::White, gf::Black };
    return DefaultStyle;
  }

  namespace {

    gf::ConsoleRichStyle compute_message_rich_style()
    {
      gf::ConsoleRichStyle style(message_default_style());

      style.set_style("gf", { gf::Orange, gf::Azure });

      style.set_style("character", gf::Chartreuse);
      style.set_style("date", gf::gray(0.75f));
      style.set_style("item", gf::Capri);
      style.set_style("weapon", gf::Yellow);
      style.set_style("cash", gf::Erin);
      style.set_style("debt", gf::Vermilion);

      style.set_style("hero", { gf::White, gf::gray(0.25f) });

      style.set_style("girl", gf::Rose);
      style.set_style("boy", gf::Azure);
      style.set_style("non_binary", gf::White);

      style.set_style("health", gf::Crimson);
      style.set_style("non_health", gf::Gray);

      style.set_style("force", ForceColor);
      style.set_style("dexterity", DexterityColor);
      style.set_style("constitution", ConstitutionColor);

      style.set_style("nightlight", NightlightColor);
      style.set_style("daylight", DaylightColor);
      style.set_style("noon", NoonColor);
      style.set_style("twilight", TwilightColor);

      return style;
    }

  }

  const gf::ConsoleRichStyle& message_rich_style()
  {
    static const gf::ConsoleRichStyle RichStyle = compute_message_rich_style();
    return RichStyle;
  }

  /*
   * UI styles (blue background)
   */

  const gf::ConsoleStyle& ui_default_style()
  {
    static constexpr gf::ConsoleStyle DefaultStyle = { gf::White, RpgBlue };
    return DefaultStyle;
  }

  namespace {

    gf::ConsoleRichStyle compute_ui_rich_style()
    {
      gf::ConsoleRichStyle style(ui_default_style());

      style.set_style("item", gf::Capri);
      style.set_style("property", gf::gray(0.7f));

      style.set_style("key", gf::Cerise);
      style.set_style("context", gf::gray(0.7f));

      style.set_style("better", gf::Green);
      style.set_style("worse", gf::Red);

      return style;
    }

  }

  const gf::ConsoleRichStyle& ui_rich_style()
  {
    static const gf::ConsoleRichStyle RichStyle = compute_ui_rich_style();
    return RichStyle;
  }

}
