/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "WeatherOptions.h"
#include "..\Menu\Language.h"

namespace sub
{
void WeatherMenu()
{
    bool spSnowOn = false;
    bool spSnowOff = false;
    bool windSpeedPlus = false;
    bool windSpeedMinus = false;
    bool rainFXPlus = false;
    bool rainFXMinus = false;
    bool wavesHeightPlus = false;
    bool wavesHeightMinus = false;
    bool gravityLevelPlus = false;
    bool gravityLevelMinus = false;
    bool clearWeatherOverride = false;

    float gravityMultiplier = GTAmemory::GetWorldGravity();
    float windSpeed = GET_WIND_SPEED();
    float wavesHeight = GET_DEEP_OCEAN_SCALER();

    AddTitle(Language::Translate("weather.title"));
    AddTickol(Language::Translate("weather.reset"), true, clearWeatherOverride, clearWeatherOverride, TICKOL::CROSS);
    AddOptionDescription(Language::Translate("weather.reset.description"));
    if (clearWeatherOverride)
    {
        World::ClearWeatherOverride();
    }
    const std::vector<std::string> weatherLabels{
        Language::Translate("weather.extraSunny"),
        Language::Translate("weather.clear"),
        Language::Translate("weather.clouds"),
        Language::Translate("weather.smog"),
        Language::Translate("weather.foggy"),
        Language::Translate("weather.overcast"),
        Language::Translate("weather.rain"),
        Language::Translate("weather.thunder"),
        Language::Translate("weather.clearing"),
        Language::Translate("weather.neutral"),
        Language::Translate("weather.snow"),
        Language::Translate("weather.blizzard"),
        Language::Translate("weather.snowLight"),
        Language::Translate("weather.halloween"),
        Language::Translate("weather.halloweenSnow"),
        Language::Translate("weather.halloweenRain")
    };
    for (size_t weatherIndex = 0; weatherIndex < World::sWeatherNames.size(); ++weatherIndex)
    {
        auto& weatherName = World::sWeatherNames[weatherIndex];
        bool bWeatherPressed = false;
        const std::string& weatherLabel = weatherIndex < weatherLabels.size() ? weatherLabels[weatherIndex] : weatherName.first;
        AddTickol(weatherLabel, GET_PREV_WEATHER_TYPE_HASH_NAME() == GET_HASH_KEY(weatherName.second), bWeatherPressed, bWeatherPressed);
        if (bWeatherPressed)
        {
            addlog(ige::LogType::LOG_DEBUG, "Setting weather to " + weatherName.first);
            World::SetWeatherOverride(weatherName.second);
        }
    }

    AddLocal(Language::Translate("weather.snowOnTerrain"), g_spSnow.IsSnow(), spSnowOn, spSnowOff);
    AddOptionDescription(Language::Translate("weather.snowOnTerrain.description"));
    AddNumber(Language::Translate("weather.windSpeed"), windSpeed, 2, null, windSpeedPlus, windSpeedMinus);
    AddNumber(Language::Translate("weather.oceanWaveStrength"), wavesHeight, 2, null, wavesHeightPlus, wavesHeightMinus);
    AddNumber(Language::Translate("weather.rainPuddles"), g_rainFXIntensity, 2, null, rainFXPlus, rainFXMinus);
    AddOptionDescription(Language::Translate("weather.rainPuddles.description"));
    AddNumber(Language::Translate("weather.gravity"), gravityMultiplier, 2, null, gravityLevelPlus, gravityLevelMinus);
    AddOptionDescription(Language::Translate("weather.gravity.description"));
    AddOption(Language::Translate("weather.cloudsMenu"), null, nullFunc, SUB::CLOUDOPS);
    AddOption(Language::Translate("weather.waterHack"), null, nullFunc, SUB::WATERHACK);
    AddOptionDescription(Language::Translate("weather.waterHack.description"));


    if (spSnowOn || spSnowOff)
    {
        g_spSnow.ToggleSnow(spSnowOn);
    }

    if (windSpeedPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "windSpeed_plus");
        windSpeed += 0.1f;
        SET_WIND_SPEED(windSpeed);
    }
    if (windSpeedMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "windSpeed_minus");
        windSpeed -= 0.1f;
        SET_WIND_SPEED(windSpeed);
    }

    if (wavesHeightPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "wavesHeight_plus");
        wavesHeight += 0.1f;
        WATER::SET_DEEP_OCEAN_SCALER(wavesHeight);
    }

    if (wavesHeightMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "wavesHeight_minus");
        wavesHeight -= 0.1f;
        WATER::SET_DEEP_OCEAN_SCALER(wavesHeight);
    }

    if (rainFXPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "rainfxit_plus");
        if (g_rainFXIntensity < 45.0f)
        {
            g_rainFXIntensity += 0.1f;
        }
        SET_RAIN(g_rainFXIntensity);
    }

    if (rainFXMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "rainfxit_minus");
        if (g_rainFXIntensity > 0.0f)
        {
            g_rainFXIntensity -= 0.1f;
        }
        SET_RAIN(g_rainFXIntensity);
    }

    if (gravityLevelPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "gravityLevel_plus");
        gravityMultiplier += 0.1;
        GTAmemory::SetWorldGravity(gravityMultiplier);
    }

    if (gravityLevelMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "gravityLevel_minus");
        gravityMultiplier -= 0.1;
        GTAmemory::SetWorldGravity(gravityMultiplier);
    }
}

namespace WeatherClouds
{
const std::array<std::string, 20> cloudNames{{{"Altostratus"},  {"Cirrocumulus"}, {"Cirrus"},    {"Clear 01"},       {"Cloudy 01"}, {"Contrails"}, {"Horizonband1"},
                                              {"Horizonband2"}, {"Horizonband3"}, {"Horsey"},    {"Nimbus"},         {"NoClouds"},  {"Puffs"},     {"Rain"},
                                              {"Shower"},       {"Snowy 01"},     {"Stormy 01"}, {"Stratoscumulus"}, {"Stripey"},   {"Wispy"}}};

void CloudMenu()
{
    AddTitle(Language::Translate("weather.clouds.title"));

    bool bResetPressed = false;
    AddTickol(Language::Translate("weather.clouds.reset"), true, bResetPressed, bResetPressed, TICKOL::CROSS);
    if (bResetPressed)
    {
        UNLOAD_ALL_CLOUD_HATS();
    }

    for (size_t cloudIndex = 0; cloudIndex < cloudNames.size(); ++cloudIndex)
    {
        auto& name = cloudNames[cloudIndex];
        bool bPressed = false;
        AddOption(Language::Translate("weather.clouds." + std::to_string(cloudIndex)), bPressed);
        if (bPressed)
        {
            addlog(ige::LogType::LOG_DEBUG, "Set Clouds: " + name);
            LOAD_CLOUD_HAT(name.c_str(), 0.5f);
        }
    }
}
} // namespace WeatherClouds
} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(WEATHEROPS, sub::WeatherMenu)
REGISTER_SUBMENU(CLOUDOPS, sub::WeatherClouds::CloudMenu)
