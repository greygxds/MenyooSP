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

    AddTitle(Language::Translate("menu.weather.title"));
    AddTickol(Language::Translate("menu.weather.option.reset.label"), true, clearWeatherOverride, clearWeatherOverride, TICKOL::CROSS);
    AddOptionDescription(Language::Translate("menu.weather.option.reset.description"));
    if (clearWeatherOverride)
    {
        World::ClearWeatherOverride();
    }
    const std::vector<std::string> weatherLabels{
        Language::Translate("menu.weather.option.extra_sunny.label"),
        Language::Translate("menu.weather.option.clear.label"),
        Language::Translate("menu.weather.option.clouds.label"),
        Language::Translate("menu.weather.option.smog.label"),
        Language::Translate("menu.weather.option.foggy.label"),
        Language::Translate("menu.weather.option.overcast.label"),
        Language::Translate("menu.weather.option.rain.label"),
        Language::Translate("menu.weather.option.thunder.label"),
        Language::Translate("menu.weather.option.clearing.label"),
        Language::Translate("menu.weather.option.neutral.label"),
        Language::Translate("menu.weather.option.snow.label"),
        Language::Translate("menu.weather.option.blizzard.label"),
        Language::Translate("menu.weather.option.snow_light.label"),
        Language::Translate("menu.weather.option.halloween.label"),
        Language::Translate("menu.weather.option.halloween_snow.label"),
        Language::Translate("menu.weather.option.halloween_rain.label")
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

    AddLocal(Language::Translate("menu.weather.option.snow_on_terrain.label"), g_spSnow.IsSnow(), spSnowOn, spSnowOff);
    AddOptionDescription(Language::Translate("menu.weather.option.snow_on_terrain.description"));
    AddNumber(Language::Translate("menu.weather.option.wind_speed.label"), windSpeed, 2, null, windSpeedPlus, windSpeedMinus);
    AddNumber(Language::Translate("menu.weather.option.ocean_wave_strength.label"), wavesHeight, 2, null, wavesHeightPlus, wavesHeightMinus);
    AddNumber(Language::Translate("menu.weather.option.rain_puddles.label"), g_rainFXIntensity, 2, null, rainFXPlus, rainFXMinus);
    AddOptionDescription(Language::Translate("menu.weather.option.rain_puddles.description"));
    AddNumber(Language::Translate("menu.weather.option.gravity.label"), gravityMultiplier, 2, null, gravityLevelPlus, gravityLevelMinus);
    AddOptionDescription(Language::Translate("menu.weather.option.gravity.description"));
    AddOption(Language::Translate("menu.weather.option.clouds_menu.label"), null, nullFunc, SUB::CLOUDOPS);
    AddOption(Language::Translate("menu.weather.option.water_hack.label"), null, nullFunc, SUB::WATERHACK);
    AddOptionDescription(Language::Translate("menu.weather.option.water_hack.description"));


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
    AddTitle(Language::Translate("menu.weather.clouds.title"));

    bool bResetPressed = false;
    AddTickol(Language::Translate("menu.weather.clouds.option.reset.label"), true, bResetPressed, bResetPressed, TICKOL::CROSS);
    if (bResetPressed)
    {
        UNLOAD_ALL_CLOUD_HATS();
    }

    for (size_t cloudIndex = 0; cloudIndex < cloudNames.size(); ++cloudIndex)
    {
        auto& name = cloudNames[cloudIndex];
        bool bPressed = false;
        AddOption(Language::TranslateStable("menu.weather.clouds.option.cloud_type.value", name, name), bPressed);
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
