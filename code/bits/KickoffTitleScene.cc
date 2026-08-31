#include "KickoffTitleScene.h"

#include <string_view>

#include <gf2/core/Color.h>
#include <gf2/core/ConsoleChar.h>
#include <gf2/core/ConsoleOperations.h>
#include <gf2/core/FontManager.h>
#include <gf2/core/Math.h>
#include <gf2/core/Range.h>

#include "FarWest.h"
#include "Styles.h"

namespace fw {

  namespace {

    constexpr gf::Color TitleColor = gf::Amber;

    constexpr std::u16string_view Title[] = {
      u"   ██████  ███   █████      ███ ███ ███ ██████  ███  ███████    █████   ███      ",
      u"   ██████  ███   █████      ███ ███ ███ ██████  ███  ███████    █████   ███      ",
      u"    █   █ █   █   █   █      █   █   █   █   █ █   █ █  █  █     █   █   █       ",
      u"    █     █   █   █   █      █   █   █   █     █        █        █   █   █       ",
      u"    █     █   █   █   █      █   █   █   █     █        █        █   █   █       ",
      u"    ███   █████   ████        █ █ █ █    ███    ███     █        ████    █       ",
      u"    ███   █████   ████        █ █ █ █    ███       █    █        ████    █       ",
      u"    █     █   █   █   █       █ █ █ █    █         █    █        █   █   █       ",
      u"    █     █   █   █   █        █   █     █         █    █        █   █   █       ",
      u"    █     █   █   █   █        █   █     █   █ █   █    █        █   █   █       ",
      u"   ███   ███ ███ ███ ███      ███ ███   ██████  ███    ███      ███ ███ ██████   ",
      u"   ███   ███ ███ ███ ███      ███ ███   ██████  ███    ███      ███ ███ ██████   ",
      u"                                                                                 ",
      // u"                                                                                 ",
      u"   ─┬─│        │                                                ┬     ┬      │   ",
      u"    │ │  ·     │                            ┼─                  │  ·  │      │   ",
      u"    │ ├─╮┬╭─╮  │╭─╮╭─╮┌─╮  ┬ ┬╭─╮╭─╮  ┌─╮╭─╮│   ╭─╮  ╭─╮╭─╮╭─╮╭─┤  ┬╭─┤╭─╮╭─╮│   ",
      u"    │ │ ││╰─╮  ││ │╭─┤│ │  │││╭─┤╰─╮  │ ││ ││   ╭─┤  │ ││ ││ ││ │  ││ │├─┘╭─┤    ",
      u"    ┴ ┴ ┴┴╰─╯  ┴╰─╯╰─┘┴ ┴  └┴┘╰─┘╰─╯  ┴ ┴╰─╯┴   ╰─┘  ╰─┤╰─╯╰─╯╰─┘  ┴╰─┘╰─╯╰─┘·   ",
      u"                                                     ╰─╯                         ",
      u"                                                                                 ",
    };

    constexpr gf::Vec2I Size = gf::vec(Title[0].size(), std::size(Title));

  }

  KickoffTitleScene::KickoffTitleScene(FarWest* game)
  : m_game(game)
  , m_title(Size)
  {
    for (const gf::Vec2I position : gf::position_range(Size)) {
      const char16_t c = Title[position.y][position.x];
      gf::console_write_picture(m_title, position, c, TitleColor);
    }
  }

  void KickoffTitleScene::render(gf::Console& console)
  {
    const gf::Vec2I padding = console.size() - m_title.size();
    const gf::Vec2I title_position = { padding.w / 2, padding.h / 2 - padding.h / 6 };
    gf::console_blit_to(m_title, console, title_position);

    const gf::Vec2I subtitle_position = { console.size().w / 2, console.size().h - 2 };
    gf::console_print_text(console, subtitle_position, gf::ConsoleAlignment::Center, message_rich_style(), "copyright © 2025-2026 — made with <style=gf>gf2</> (and no AI)");
  }

}
