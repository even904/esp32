#ifndef CUSTOM_FONT_H
#define CUSTOM_FONT_H

#include "lvgl.h"
// Other custom font
#include "WeatherIcon.h"
// Font declarations
LV_FONT_DECLARE(WenQuanWeiMiHei_48);  // default 48px, Only ascii : 0x0020-0x007E❗
LV_FONT_DECLARE(WenQuanWeiMiHei_36);  // default 36px, do not support chinese punctuation❗
LV_FONT_DECLARE(WenQuanWeiMiHei_24);  // default 24px, do not support chinese punctuation❗
LV_FONT_DECLARE(WenQuanWeiMiHei_18);  // default 18px, do not support chinese punctuation❗
LV_FONT_DECLARE(WenQuanWeiMiHei_12);  // default 12px, do not support chinese punctuation❗


LV_FONT_DECLARE(MoreIcon);
#define MI18_Calendar      "\xee\x98\x91"  // 0xE611 MoreIcon:
#define MI18_WindPower     "\xee\x98\xab"  // 0xE62B MoreIcon:
#define MI18_Locate        "\xee\x98\xb4"  // 0xE634 MoreIcon:
#define MI18_WindDirection "\xee\x99\xb9"  // 0xE679 MoreIcon:
#define MI18_UpdateTime    "\xee\x9f\xa6"  // 0xE7E6 MoreIcon:
#define MI18_Temperature   "\xee\xa0\x8f"  // 0xE80F MoreIcon:
#define MI18_Humidity      "\xee\xac\xba"  // 0xEB3A MoreIcon:

#endif