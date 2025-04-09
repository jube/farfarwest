#include "TitleScene.h"

#include <string_view>

#include <gf2/core/Color.h>
#include <gf2/core/ConsoleChar.h>
#include <gf2/core/FontManager.h>
#include <gf2/core/Math.h>
#include <gf2/core/Range.h>

#include "FarFarWest.h"

namespace ffw {

  namespace {

    constexpr gf::Color TitleColor = gf::Amber;

    constexpr std::u16string_view Title[] = {
      u" ██████  ███   █████     ██████  ███   █████     ███ ███ ███ ██████  ███  ███████",
      u" ██████  ███   █████     ██████  ███   █████     ███ ███ ███ ██████  ███  ███████",
      u"  █   █ █   █   █   █     █   █ █   █   █   █     █   █   █   █   █ █   █ █  █  █",
      u"  █     █   █   █   █     █     █   █   █   █     █   █   █   █     █        █   ",
      u"  █     █   █   █   █     █     █   █   █   █     █   █   █   █     █        █   ",
      u"  ███   █████   ████      ███   █████   ████       █ █ █ █    ███    ███     █   ",
      u"  ███   █████   ████      ███   █████   ████       █ █ █ █    ███       █    █   ",
      u"  █     █   █   █   █     █     █   █   █   █      █ █ █ █    █         █    █   ",
      u"  █     █   █   █   █     █     █   █   █   █       █   █     █         █    █   ",
      u"  █     █   █   █   █     █     █   █   █   █       █   █     █   █ █   █    █   ",
      u" ███   ███ ███ ███ ███   ███   ███ ███ ███ ███     ███ ███   ██████  ███    ███  ",
      u" ███   ███ ███ ███ ███   ███   ███ ███ ███ ███     ███ ███   ██████  ███    ███  ",
      u"                                                                                 ",
      // u"                                                                                 ",
      u"─┬─│         │                                                    ┬      ┬      │",
      u" │ │  ·      │                              ┼─                    │   ·  │      │",
      u" │ ├─┐┬┌─┐   │┌─┐┌─┐┌─┐   ┬ ┬┌─┐┌─┐   ┌─┐┌─┐│    ┌─┐   ┌─┐┌─┐┌─┐┌─┤   ┬┌─┤┌─┐┌─┐│",
      u" │ │ ││└─┐   ││ │┌─┤│ │   │││┌─┤└─┐   │ ││ ││    ┌─┤   │ ││ ││ ││ │   ││ │├─┘┌─┤ ",
      u" ┴ ┴ ┴┴└─┘   ┴└─┘└─┘┴ ┴   └┴┘└─┘└─┘   ┴ ┴└─┘┴    └─┘   └─┤└─┘└─┘└─┘   ┴└─┘└─┘└─┘·",
      u"                                                       ──┘                       ",
      u"                                                                                 ",
      // u"       ─┬─│         │                              ┬       ┬      ┬      │       ",
      // u"        │ │  ·      │                              │       │   ·  │      │       ",
      // u"        │ ├─┐┬┌─┐   │┌─┐┌─┐┌─┐   ┬ ┬┌─┐┌─┐   ┌─┐   ├─┐┌─┐┌─┤   ┬┌─┤┌─┐┌─┐│       ",
      // u"        │ │ ││└─┐   ││ │┌─┤│ │   │││┌─┤└─┐   ┌─┤   │ │┌─┤│ │   ││ │├─┘┌─┤        ",
      // u"        ┴ ┴ ┴┴└─┘   ┴└─┘└─┘┴ ┴   └┴┘└─┘└─┘   └─┘   └─┘└─┘└─┘   ┴└─┘└─┘└─┘·       ",
    };

    constexpr gf::Vec2I Size = gf::vec(Title[0].size(), std::size(Title));

  }

  TitleScene::TitleScene(FarFarWest* game)
  : m_game(game)
  , m_title(Size)
  {
    for (const gf::Vec2I position : gf::position_range(Size)) {
      const char16_t c = Title[position.y][position.x];
      m_title.put_character(position, c, TitleColor, gf::Transparent);
    }
  }

  void TitleScene::render(gf::Console& console)
  {
    const gf::Vec2I padding = console.size() - m_title.size();
    const gf::Vec2I title_position = { padding.w / 2, padding.h / 2 - padding.h / 6 };
    m_title.blit_to(console, gf::RectI::from_size(m_title.size()), title_position);

    // gf::ConsoleStyle style;
    // style.color.foreground = gf::Amber;
    //
    // const gf::Vec2I subtitle_position = { console.size().w / 2, title_position.y + Size.y + 2 };
    // console.print(subtitle_position, gf::ConsoleAlignment::Center, style, "This mortgage was not a good idea. Find the money or run!");

    gf::ConsoleStyle style;
    style.color.foreground = gf::White;

    const gf::Vec2I subtitle_position = { console.size().w / 2, console.size().h - 2 };
    console.print(subtitle_position, gf::ConsoleAlignment::Center, m_game->style(), "copyright (c) 2025 ─ made with <style=gf>gf2</>");
  }

}
