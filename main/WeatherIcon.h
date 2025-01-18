#ifndef WEATHER_ICON_H
#define WEATHER_ICON_H
#include "lvgl.h"

#define WI84_ee9880  "\xee\x98\x80" // 0xE600 WeatherIcon: Fog
#define WI84_ee9881  "\xee\x98\x81" // 0xE601 WeatherIcon: Hot
#define WI84_ee9882  "\xee\x98\x82" // 0xE602 WeatherIcon: Breezee
#define WI84_ee9887  "\xee\x98\x87" // 0xE607 WeatherIcon: StrongWind
#define WI84_ee9891  "\xee\x98\x91" // 0xE611 WeatherIcon: HeavyFog
#define WI84_ee9892  "\xee\x98\x92" // 0xE612 WeatherIcon: HardRain
#define WI84_ee9893  "\xee\x98\x93" // 0xE613 WeatherIcon: RainAndSnow
#define WI84_ee9894  "\xee\x98\x94" // 0xE614 WeatherIcon: DaySnow
#define WI84_ee9895  "\xee\x98\x95" // 0xE615 WeatherIcon: NightSnow
#define WI84_ee9898  "\xee\x98\x98" // 0xE618 WeatherIcon: LightRain
#define WI84_ee9899  "\xee\x98\x99" // 0xE619 WeatherIcon: HeavyRain
#define WI84_ee989a  "\xee\x98\x9a" // 0xE61A WeatherIcon: MiddleRain
#define WI84_ee989d  "\xee\x98\x9d" // 0xE61D WeatherIcon: ThunderShower
#define WI84_ee98a8  "\xee\x98\xa8" // 0xE628 WeatherIcon: Dust
#define WI84_ee98aa  "\xee\x98\xaa" // 0xE62A WeatherIcon: Haze
#define WI84_ee98b4  "\xee\x98\xb4" // 0xE634 WeatherIcon: DustStorm
#define WI84_ee98b6  "\xee\x98\xb6" // 0xE636 WeatherIcon: LightSnow
#define WI84_ee98b8  "\xee\x98\xb8" // 0xE638 WeatherIcon: MiddleSnow
#define WI84_ee98b9  "\xee\x98\xb9" // 0xE639 WeatherIcon: HardSnow
#define WI84_ee98bc  "\xee\x98\xbc" // 0xE63C WeatherIcon: HeavySnow
#define WI84_ee9990  "\xee\x99\x90" // 0xE650 WeatherIcon: Snowy
#define WI84_ee99a2  "\xee\x99\xa2" // 0xE662 WeatherIcon: DaySunny
#define WI84_ee9b91  "\xee\x9b\x91" // 0xE6D1 WeatherIcon: Storm
#define WI84_ee9b93  "\xee\x9b\x93" // 0xE6D3 WeatherIcon: NightCloudy
#define WI84_ee9bb2  "\xee\x9b\xb2" // 0xE6F2 WeatherIcon: UnknownWeather
#define WI84_ee9bb7  "\xee\x9b\xb7" // 0xE6F7 WeatherIcon: Shower
#define WI84_ee9ca2  "\xee\x9c\xa2" // 0xE722 WeatherIcon: StrongThunderShower
#define WI84_ee9d84  "\xee\x9d\x84" // 0xE744 WeatherIcon: DayCloudy
#define WI84_ee9d99  "\xee\x9d\x99" // 0xE759 WeatherIcon: Overcast
#define WI84_ee9e86  "\xee\x9e\x86" // 0xE786 WeatherIcon: NightSunny
#define WI84_ee9f96  "\xee\x9f\x96" // 0xE7D6 WeatherIcon: Tornado
#define WI84_eea38e  "\xee\xa3\x8e" // 0xE8CE WeatherIcon: Cold

#define GET_WEATHER_ICON(index) WI84_##index

LV_FONT_DECLARE(WeatherIcon);

#endif