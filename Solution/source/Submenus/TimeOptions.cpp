/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "TimeOptions.h"
#include "..\Menu\Language.h"

namespace sub
{
void TimeMenu()
{
    bool pauseClockToggle = false;
    bool hourPlus = false;
    bool hourMinus = false;
    bool minutePlus = false;
    bool minuteMinus = false;
    bool timeScalePlus = false;
    bool timeScaleMinus = false;

    int hour = GET_CLOCK_HOURS();
    int minute = GET_CLOCK_MINUTES();

    bool presetSunrise = false, presetNoon = false, presetSunset = false, presetNight = false;

    AddTitle(Language::Translate("time.title"));
    AddOption(Language::Translate("time.clockSystem"), null, nullFunc, SUB::CLOCK);
    AddToggle(Language::Translate("time.pauseClock"), pauseClock, pauseClockToggle);
    AddOptionDescription(Language::Translate("time.pauseClock.description"));
    AddToggle(Language::Translate("time.syncSystem"), syncClock);
    AddOptionDescription(Language::Translate("time.syncSystem.description"));
    AddNumber(Language::Translate("time.hour"), hour, 0, null, hourPlus, hourMinus);
    AddNumber(Language::Translate("time.minute"), minute, 0, null, minutePlus, minuteMinus);
    AddNumber(Language::Translate("time.worldSpeed"), currentTimescale, 1, null, timeScalePlus, timeScaleMinus);
    AddOptionDescription(Language::Translate("time.worldSpeed.description"));
    AddBreak(Language::Translate("time.presets"));
    AddOption(Language::Translate("time.sunrise"), presetSunrise);
    AddOption(Language::Translate("time.noon"), presetNoon);
    AddOption(Language::Translate("time.sunset"), presetSunset);
    AddOption(Language::Translate("time.night"), presetNight);

    if (presetSunrise)
    {
        NETWORK_OVERRIDE_CLOCK_TIME(6, 0, 0);
        if (pauseClock)
        {
            pauseClockH = 6;
            pauseClockM = 0;
        }
        return;
    }
    if (presetNoon)
    {
        NETWORK_OVERRIDE_CLOCK_TIME(12, 0, 0);
        if (pauseClock)
        {
            pauseClockH = 12;
            pauseClockM = 0;
        }
        return;
    }
    if (presetSunset)
    {
        NETWORK_OVERRIDE_CLOCK_TIME(19, 0, 0);
        if (pauseClock)
        {
            pauseClockH = 19;
            pauseClockM = 0;
        }
        return;
    }
    if (presetNight)
    {
        NETWORK_OVERRIDE_CLOCK_TIME(23, 0, 0);
        if (pauseClock)
        {
            pauseClockH = 23;
            pauseClockM = 0;
        }
        return;
    }

    if (hourPlus)
    {
        if ((hour + 1) == 24)
        {
            if (GET_CLOCK_MONTH() + 1 == 12 && GET_CLOCK_DAY_OF_MONTH() + 1 == 30)
            {
                SET_CLOCK_DATE(1, 0, GET_CLOCK_YEAR() + 1);
            }
            else if (GET_CLOCK_DAY_OF_MONTH() + 1 == 30)
            {
                SET_CLOCK_DATE(1, GET_CLOCK_MONTH() + 1, GET_CLOCK_YEAR());
            }
            else
            {
                SET_CLOCK_DATE(GET_CLOCK_DAY_OF_MONTH() + 1, GET_CLOCK_MONTH(), GET_CLOCK_YEAR());
            }
            NETWORK_OVERRIDE_CLOCK_TIME(0, minute, GET_CLOCK_SECONDS());
        }
        else
        {
            NETWORK_OVERRIDE_CLOCK_TIME((hour + 1), minute, GET_CLOCK_SECONDS());
        }
        return;
    }
    if (hourMinus)
    {
        if ((hour - 1) == -1)
        {
            if (GET_CLOCK_MONTH() - 1 == -1 && GET_CLOCK_DAY_OF_MONTH() - 1 == 0)
            {
                SET_CLOCK_DATE(28, 11, GET_CLOCK_YEAR() - 1);
            }
            else if (GET_CLOCK_DAY_OF_MONTH() - 1 == 0)
            {
                SET_CLOCK_DATE(28, GET_CLOCK_MONTH() - 1, GET_CLOCK_YEAR());
            }
            else
            {
                SET_CLOCK_DATE(GET_CLOCK_DAY_OF_MONTH() - 1, GET_CLOCK_MONTH(), GET_CLOCK_YEAR());
            }
            NETWORK_OVERRIDE_CLOCK_TIME(23, minute, GET_CLOCK_SECONDS());
        }
        else
        {
            NETWORK_OVERRIDE_CLOCK_TIME((hour - 1), minute, GET_CLOCK_SECONDS());
        }
        return;
    }

    if (minutePlus)
    {
        if ((minute + 1) == 60)
        {
            if (hour + 1 == 24)
            {
                if (GET_CLOCK_DAY_OF_MONTH() + 1 == 30)
                {
                    SET_CLOCK_DATE(1, GET_CLOCK_MONTH() + 1, GET_CLOCK_YEAR());
                }
                else
                {
                    SET_CLOCK_DATE(GET_CLOCK_DAY_OF_MONTH() + 1, GET_CLOCK_MONTH(), GET_CLOCK_YEAR());
                }
                NETWORK_OVERRIDE_CLOCK_TIME(0, 0, GET_CLOCK_SECONDS());
            }
            else
            {
                NETWORK_OVERRIDE_CLOCK_TIME(hour + 1, 0, GET_CLOCK_SECONDS());
            }
        }
        else
        {
            NETWORK_OVERRIDE_CLOCK_TIME(hour, minute + 1, GET_CLOCK_SECONDS());
        }
        return;
    }
    if (minuteMinus)
    {
        if ((minute - 1) == -1)
        {
            if (hour - 1 == -1)
            {
                if (GET_CLOCK_DAY_OF_MONTH() - 1 == 0)
                {
                    SET_CLOCK_DATE(29, GET_CLOCK_MONTH() - 1, GET_CLOCK_YEAR());
                }
                else
                {
                    SET_CLOCK_DATE(GET_CLOCK_DAY_OF_MONTH() - 1, GET_CLOCK_MONTH(), GET_CLOCK_YEAR());
                }
                NETWORK_OVERRIDE_CLOCK_TIME(23, 59, GET_CLOCK_SECONDS());
            }
            else
            {
                NETWORK_OVERRIDE_CLOCK_TIME(hour - 1, 59, GET_CLOCK_SECONDS());
            }
        }
        else
        {
            NETWORK_OVERRIDE_CLOCK_TIME(hour, minute - 1, GET_CLOCK_SECONDS());
        }
        return;
    }

    if (timeScalePlus && currentTimescale < 1.1f)
    {
        currentTimescale += 0.1f;
        SET_TIME_SCALE(currentTimescale);
        return;
    }

    if (timeScaleMinus && currentTimescale > 0.0f)
    {
        currentTimescale -= 0.1f;
        SET_TIME_SCALE(currentTimescale);
        return;
    }

    if (pauseClockToggle)
    {
        pauseClockH = GET_CLOCK_HOURS();
        pauseClockM = GET_CLOCK_MINUTES();
        return;
    }
}

namespace Clock
{
UINT8 loopClock = 0;
UINT8 analogueClockIndex = 0;
Vector2 analogueClockPosition = {0.931f, 0.126f};

const std::string weekDayNames[] = {
    "time.weekday.sunday", "time.weekday.monday", "time.weekday.tuesday", "time.weekday.wednesday", "time.weekday.thursday", "time.weekday.friday", "time.weekday.saturday"
};

inline void DisplayClockDigital()
{
    time_t now = time(0);
    tm t;
    localtime_s(&t, &now);

    char mintBuff[3];
    sprintf_s(mintBuff, "%02d", t.tm_min);

    Game::Print::SetupDraw(GTAfont::Arial, Vector2(0.3f, 0.3f), true, false, true, RGBA(255, 255, 255, 210));
    Game::Print::DrawString(
        oss_ << Language::Translate(weekDayNames[t.tm_wday]) << " - " << (t.tm_hour < 13 ? t.tm_hour : t.tm_hour - 12) << ':' << mintBuff << (t.tm_hour < 12 ? " am" : " pm"), 0.5f, 0.007f
    ); //0.01f, 0.8f);
}

std::vector<ClockImage> clockImages;

void LoadClockImages()
{
    std::vector<std::string> results;
    get_all_filenames_with_extension(GetPathffA(Pathff::Graphics, true) + "Clock", ".png", results, false);
    for (auto sit = results.begin(); sit != results.end();)
    {
        if (sit->back() == 'e') // ..._face
        {
            std::string fullNameWithoutExtension = sit->substr(0, sit->rfind('_'));
            std::string faceName = *sit + ".png"; //fullNameWithoutExtension + "_face.png";
            std::string handhName = fullNameWithoutExtension + "_handh.png";
            std::string handmName = fullNameWithoutExtension + "_handm.png";

            DxHookIMG::DxTexture faceId = GetPathffA(Pathff::Graphics, true) + "Clock\\" + faceName;
            DxHookIMG::DxTexture hourId = GetPathffA(Pathff::Graphics, true) + "Clock\\" + handhName;
            DxHookIMG::DxTexture minuteId = GetPathffA(Pathff::Graphics, true) + "Clock\\" + handmName;

            ClockImage cmg = {fullNameWithoutExtension, faceId, hourId, minuteId};
            clockImages.push_back(cmg);
        }
        ++sit;
    }
}

inline void DisplayClockAnalogue()
{
    time_t now = time(0);
    tm t;
    localtime_s(&t, &now);

    auto& cmg = clockImages[analogueClockIndex];

    const Vector2& size = Vector2(0.1540f, 0.164f) * 0.7f;
    const Vector2& pos = analogueClockPosition;

    cmg.faceId.Draw(0, pos, size, 0.0f, RGBA::AllWhite());
    cmg.hourId.Draw(0, pos, size, (30.0f * t.tm_hour) + (0.5f * t.tm_min), RGBA::AllWhite());
    cmg.minuteId.Draw(0, pos, size, (6.0f * t.tm_min), RGBA::AllWhite());
}

void DisplayClock()
{
    switch (loopClock)
    {
    case 1:
        DisplayClockDigital();
        break; // Digital
    case 2:
        DisplayClockAnalogue();
        break; // Analogue
    }
}

void ClockMenu()
{
    AddTitle(Language::Translate("time.clock.title"));

    auto& mode = loopClock;

    UINT minClockTime = 0;
    UINT8 maxClockTime = 2;
    bool clockTimePlus = false;
    bool clockTimeMinus = false;
    AddTexter(
        Language::Translate("time.clock.mode"),
        mode,
        std::vector<std::string>{Language::Translate("time.clock.off"), Language::Translate("time.clock.digital"), Language::Translate("time.clock.analogue")},
        null,
        clockTimePlus,
        clockTimeMinus
    );
    AddOptionDescription(Language::Translate("time.clock.mode.description"));
    if (clockTimePlus)
    {
        if (mode < maxClockTime)
            mode++;
    }
    if (clockTimeMinus)
    {
        if (mode > minClockTime)
            mode--;
    }

    if (mode == 2) // If analogue
    {
        AddBreak(Language::Translate("time.clock.themes"));
        for (UINT i = 0; i < clockImages.size(); i++)
        {
            bool bAcmgPressed = false;
            AddTickol(clockImages[i].name, clockImages[analogueClockIndex].faceId == clockImages[i].faceId, bAcmgPressed, bAcmgPressed);
            if (bAcmgPressed)
            {
                analogueClockIndex = i;
            }
        }

        AddBreak(Language::Translate("time.clock.position"));

        bool analogueClockPosXPlus = false;
        bool analogueClockXMinus = false;
        bool analogueClockPosYPlus = false;
        bool analogueClockPosYMinus = false;
        AddNumber(Language::Translate("time.clock.x"), analogueClockPosition.x, 3, null, analogueClockPosXPlus, analogueClockXMinus);
        AddNumber(Language::Translate("time.clock.y"), analogueClockPosition.y, 3, null, analogueClockPosYPlus, analogueClockPosYMinus);

        if (analogueClockPosXPlus)
        {
            analogueClockPosition.x += 0.005f;
        }
        else if (analogueClockXMinus)
        {
            analogueClockPosition.x -= 0.005f;
        }
        if (analogueClockPosYPlus)
        {
            analogueClockPosition.y += 0.005f;
        }
        else if (analogueClockPosYMinus)
        {
            analogueClockPosition.y -= 0.005f;
        }
    }
}
} // namespace Clock
} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(TIMEOPS, sub::TimeMenu)
REGISTER_SUBMENU(CLOCK, sub::Clock::ClockMenu)
