#include "Pictures.h"

#include <cassert>

namespace ffw {

  char16_t rotate_picture(char16_t picture, gf::Direction direction)
  {
    assert(direction != gf::Direction::Center);
    const int8_t index = static_cast<int8_t>(direction);
    assert(0 <= index && index < 4);

    // URDL
    switch (picture) {
      case u'▀':
        return u"▀▐▄▌"[index];
      case u'▐':
        return u"▐▄▌▀"[index];
      case u'▄':
        return u"▄▌▀▐"[index];
      case u'▌':
        return u"▌▀▐▄"[index];
      // double straight
      case u'║':
        return u"║═║═"[index];
      case u'═':
        return u"═║═║"[index];
      // double T
      case u'╣':
        return u"╣╩╠╦"[index];
      case u'╩':
        return u"╩╠╦╣"[index];
      case u'╠':
        return u"╠╦╣╩"[index];
      case u'╦':
        return u"╦╣╩╠"[index];
      // double corner
      case u'╚':
        return u"╚╔╗╝"[index];
      case u'╔':
        return u"╔╗╝╚"[index];
      case u'╗':
        return u"╗╝╚╔"[index];
      case u'╝':
        return u"╝╚╔╗"[index];
      // double and single T
      case u'╢':
        return u"╢╧╟╤"[index];
      case u'╧':
        return u"╧╟╤╢"[index];
      case u'╟':
        return u"╟╤╢╧"[index];
      case u'╤':
        return u"╤╢╧╟"[index];
      // single and double T
      case u'╡':
        return u"╡╨╞╥"[index];
      case u'╨':
        return u"╨╞╥╡"[index];
      case u'╞':
        return u"╞╥╡╨"[index];
      case u'╥':
        return u"╥╡╨╞"[index];
      // double and single cross
      case u'╫':
        return u"╫╪╫╪"[index];
      case u'╪':
        return u"╪╫╪╫"[index];
      // double and single corner
      case u'╓':
        return u"╓╕╜╘"[index];
      case u'╕':
        return u"╕╜╘╓"[index];
      case u'╜':
        return u"╜╘╓╕"[index];
      case u'╘':
        return u"╘╓╕╜"[index];
      case u'╖':
        return u"╖╛╙╒"[index];
      case u'╛':
        return u"╛╙╒╖"[index];
      case u'╙':
        return u"╙╒╖╛"[index];
      case u'╒':
        return u"╒╖╛╙"[index];
      // single straight
      case u'│':
        return u"│─│─"[index];
      case u'─':
        return u"─│─│"[index];
      // single T
      case u'┤':
        return u"┤┴├┬"[index];
      case u'┴':
        return u"┴├┬┤"[index];
      case u'├':
        return u"├┬┤┴"[index];
      case u'┬':
        return u"┬┤┴├"[index];
      // single corner
      case u'└':
        return u"└┌┐┘"[index];
      case u'┌':
        return u"┌┐┘└"[index];
      case u'┐':
        return u"┐┘└┌"[index];
      case u'┘':
        return u"┘└┌┐"[index];

      // case u' ':
      //   return u"    "[index];

      default:
        break;
    }

    return picture;
  }

}
