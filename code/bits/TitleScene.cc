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

    constexpr std::string_view Title[] = {
      "######  ###   #####     ######  ###   #####     ### ### ### ######  ###  #######",
      "######  ###   #####     ######  ###   #####     ### ### ### ######  ###  #######",
      " #   # #   #   #   #     #   # #   #   #   #     #   #   #   #   # #   # #  #  #",
      " #     #   #   #   #     #     #   #   #   #     #   #   #   #     #        #   ",
      " #     #   #   #   #     #     #   #   #   #     #   #   #   #     #        #   ",
      " ###   #####   ####      ###   #####   ####       # # # #    ###    ###     #   ",
      " ###   #####   ####      ###   #####   ####       # # # #    ###       #    #   ",
      " #     #   #   #   #     #     #   #   #   #      # # # #    #         #    #   ",
      " #     #   #   #   #     #     #   #   #   #       #   #     #         #    #   ",
      " #     #   #   #   #     #     #   #   #   #       #   #     #   # #   #    #   ",
      "###   ### ### ### ###   ###   ### ### ### ###     ### ###   ######  ###    ###  ",
      "###   ### ### ### ###   ###   ### ### ### ###     ### ###   ######  ###    ###  ",
    };

    constexpr gf::Vec2I Size = gf::vec(Title[0].size(), std::size(Title));

  }

  TitleScene::TitleScene(FarFarWest* game)
  : m_game(game)
  {
    gf::Console console(Size);

    for (const gf::Vec2I position : gf::position_range(Size)) {
      const char16_t c = Title[position.y][position.x] == '#' ? gf::ConsoleChar::FullBlock : u' ';
      console.put_character(position, c, TitleColor, gf::Transparent);
    }

    m_title = std::move(console);
  }

  void TitleScene::render(gf::Console& buffer)
  {
    const gf::Vec2I fill = buffer.size() - m_title.size();
    const gf::Vec2I position = { fill.w / 2, fill.h / 2 - fill.h / 6 };
    m_title.blit_to(buffer, gf::RectI::from_size(m_title.size()), position);

    gf::ConsoleStyle style;
    style.color.foreground = gf::Amber;

    buffer.print({ buffer.size().w / 2, fill.h / 2 - fill.h / 6 + Size.y + 1 }, gf::ConsoleAlignment::Center, style, "This mortgage was not a good idea. Find the money or run!");

    style.color.foreground = gf::White;
    buffer.print({ 35, 35 }, gf::ConsoleAlignment::Left, style, "Start a new adventure");
    buffer.print({ 35, 36 }, gf::ConsoleAlignment::Left, style, "Continue the previous adventure");
    buffer.print({ 35, 37 }, gf::ConsoleAlignment::Left, style, "Quit");

    buffer.print({ 33, 35 }, gf::ConsoleAlignment::Left, style, ">");
  }

}
