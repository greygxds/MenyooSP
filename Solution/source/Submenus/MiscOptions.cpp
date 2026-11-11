/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "MiscOptions.h"
#include "..\Menu\Language.h"
#include "..\Menu\MenuConfig.h"
#include "..\Menu\Keybinds.h"
#include "..\Misc\FreeCam.h"
#include "Spooner/SpoonerCamera.h"
#include "Spooner/Submenus.h"
#include "VehicleModShop.h"

namespace sub
{
void MiscOps()
{
    g_activePedHandle = PLAYER_PED_ID();

    bool blackoutOff = false;
    bool miscPedPopulationOff = false;
    bool miscVehiclePopulationOff = false;

    AddTitle(Language::Translate("menu.misc.title"));
    AddOption(Language::Translate("menu.misc.option.free_cam_settings.label"), null, nullFunc, SUB::FREECAMSETTINGS);
    AddLocal(Language::Translate("menu.misc.option.manual_respawn.label"), ManualRespawn::g_manualRespawn.Enabled(), ManualRespawn::ToggleOnOff, ManualRespawn::ToggleOnOff);
    AddOptionDescription(Language::Translate("menu.misc.option.manual_respawn.description"));
    AddToggle(Language::Translate("menu.misc.option.simple_blackout.label"), simpleBlackoutMode, null, blackoutOff);
    AddOptionDescription(Language::Translate("menu.misc.option.simple_blackout.description"));
    AddToggle(Language::Translate("menu.misc.option.restricted_area_access.label"), restrictedAreasAccess);
    AddOptionDescription(Language::Translate("menu.misc.option.restricted_area_access.description"));
    AddToggle(Language::Translate("menu.misc.option.decreased_ped_population.label"), pedPopulation, null, miscPedPopulationOff);
    AddOptionDescription(Language::Translate("menu.misc.option.decreased_ped_population.description"));
    AddToggle(Language::Translate("menu.misc.option.decreased_vehicle_population.label"), vehiclePopulation, null, miscVehiclePopulationOff);
    AddOptionDescription(Language::Translate("menu.misc.option.decreased_vehicle_population.description"));
    AddToggle(Language::Translate("menu.misc.option.decreased_weapon_pickups.label"), clearWeaponPickups);
    AddOptionDescription(Language::Translate("menu.misc.option.decreased_weapon_pickups.description"));
    AddOption(Language::Translate("menu.misc.option.cutscene_player.label"), null, nullFunc, SUB::CUTSCENEPLAYER);
    AddOption(Language::Translate("menu.misc.option.tv_player.label"), null, nullFunc, SUB::TVCHANNELSTUFF_TV);
    AddOption(Language::Translate("menu.misc.option.radio.label"), null, nullFunc, SUB::RADIOSUB);
    AddOption(Language::Translate("menu.misc.option.clear_area.label"), null, nullFunc, SUB::CLEARAREA);
    AddOptionDescription(Language::Translate("menu.misc.option.clear_area.description"));
    AddOption(Language::Translate("menu.misc.option.vision_hax.label"), null, nullFunc, SUB::TIMECYCLES);
    AddOptionDescription(Language::Translate("menu.misc.option.vision_hax.description"));
    AddOption(Language::Translate("menu.misc.option.map_mods.label"), null, nullFunc, SUB::MAPMODS);
    AddOptionDescription(Language::Translate("menu.misc.option.map_mods.description"));
    AddOption(Language::Translate("menu.misc.option.hud_options.label"), null, nullFunc, SUB::HUDOPTIONS);
    AddOption(Language::Translate("menu.misc.option.game_camera_options.label"), null, nullFunc, SUB::GAMECAMOPTIONS);

    AddBreak(Language::Translate("menu.misc.section.not_so_fun.label"));

    bool bDeleteAllCams = false;
    AddOption(Language::Translate("menu.misc.option.delete_all_cameras.label"), bDeleteAllCams);
    AddOptionDescription(Language::Translate("menu.misc.option.delete_all_cameras.description"));
    if (bDeleteAllCams)
    {
        World::SetRenderingCamera(0);
        World::DestroyAllCameras();
        World::SetRenderingCamera(0);
    }

    AddOption(Language::Translate("menu.misc.option.rectangle_draw_tool.label"), null, DrawToolsMenu, -1, true);
    AddOptionDescription(Language::Translate("menu.misc.option.rectangle_draw_tool.description"));

    bool bEnableCellphoneYsc = false;
    AddTickol(
        Language::Translate("menu.misc.option.mobile_phone.label"),
        GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(0xF292D030) > 0,
        bEnableCellphoneYsc,
        bEnableCellphoneYsc,
        TICKOL::BOXTICK,
        TICKOL::BOXBLANK
    );
    AddOptionDescription(Language::Translate("menu.misc.option.mobile_phone.description"));
    if (bEnableCellphoneYsc)
    {
        if (GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(0xF292D030) > 0) // cellphone_controller
        {
            TERMINATE_ALL_SCRIPTS_WITH_THIS_NAME("cellphone_controller");
        }
        else
        {
            Game::RequestScript("cellphone_controller", 1424);
        }
    }

    enum eYscScriptTexterIndex
    {
        YSCSCRIPTTEXTER_LOAD,
        YSCSCRIPTTEXTER_UNLOAD
    };

    static UINT8 yscScriptTexterIndex = YSCSCRIPTTEXTER_LOAD;
    std::vector<std::string> vYscScriptTexter{Language::Translate("menu.misc.ysc_script.option.load.label"), Language::Translate("menu.misc.ysc_script.option.unload.label")};
    bool yscScriptInput = false;
    bool yscScriptPlus = false;
    bool yscScriptMinus = false;

    AddTexter(Language::Translate("menu.misc.option.ysc_script.label"), yscScriptTexterIndex, vYscScriptTexter, yscScriptInput, yscScriptPlus, yscScriptMinus);
    AddOptionDescription(Language::Translate("menu.misc.option.ysc_script.description"));

    if (yscScriptPlus)
    {
        if (yscScriptTexterIndex < vYscScriptTexter.size() - 1)
        {
            yscScriptTexterIndex++;
        }
    }

    if (yscScriptMinus)
    {
        if (yscScriptTexterIndex > 0)
        {
            yscScriptTexterIndex--;
        }
    }

    if (yscScriptInput)
    {
        std::string inputStr = std::string(Game::InputBox("", 65U, Language::Translate("menu.misc.ysc_script.option.input_prompt.prompt").c_str()));
        if (inputStr.length() > 0)
        {
            static const std::map<std::string, UINT16> vYscStackSizes{
                {"achievement_controller", 1424},
                {"ambient_sonar", 1424},
                {"ambient_tonya", 1424},
                {"ambient_tonyacall2", 1424},
                {"ambient_tonyacall5", 1424},
                {"am_mp_property_int", 11048},
                {"am_mp_yacht", 5000},
                {"am_pi_menu", 9800},
                {"appchecklist", 2552},
                {"appchecklist", 3800},
                {"appcontacts", 3800},
                {"appinternet", 3800},
                {"appmpjoblistnew", 2552},
                {"appmpjoblistnew", 3800},
                {"blip_controller", 1424},
                {"bootycallhandler", 1424},
                {"buddydeathresponse", 1424},
                {"building_controller", 1424},
                {"candidate_controller", 1424},
                {"carwash2", 1424},
                {"celebrations", 3650},
                {"cellphone_controller", 1424},
                {"cellphone_flashhand", 1424},
                {"cheat_controller", 1424},
                {"completionpercentage_controller", 1424},
                {"context_controller", 1424},
                {"controller_ambientarea", 1424},
                {"controller_races", 1424},
                {"controller_towing", 1424},
                {"country_race", 3650},
                {"dialogue_handler", 1424},
                {"drunk_controller", 1424},
                {"email_controller", 1424},
                {"emergencycall", 512},
                {"emergencycalllauncher", 1424},
                {"event_controller", 1424},
                {"fake_interiors", 1424},
                {"flow_controller", 1424},
                {"fmmc_launcher", 14000},
                {"fm_capture_creator", 18000},
                {"fm_deathmatch_creator", 18000},
                {"fm_lts_creator", 18000},
                {"fm_maintain_cloud_header_data", 1424},
                {"fm_main_menu", 3650},
                {"fm_mission_controller", 31000},
                {"fm_mission_creator", 18000},
                {"fm_race_creator", 18000},
                {"freemode", 21512},
                {"freemode_init", 3650},
                {"ingamehud", 3650},
                {"maintransition", 8032},
                {"maude_postbailbond", 1424},
                {"mission_stat_alerter", 1424},
                {"mission_stat_watcher", 1828},
                {"mpstatsinit", 1424},
                {"mrsphilips2", 18000},
                {"net_cloud_mission_loader", 2050},
                {"net_rank_tunable_loader", 1424},
                {"net_tunable_check", 1424},
                {"pickup_controller", 1424},
                {"player_controller", 1424},
                {"postrc_barry1and2", 1424},
                {"randomchar_controller", 1424},
                {"restrictedareas", 1424},
                {"selector", 1424},
                {"shop_controller", 1424},
                {"social_controller", 1828},
                {"stats_controller", 1424},
                {"stock_controller", 1424},
                {"taxilauncher", 1424},
                {"taxiservice", 1828},
                {"tennis_family", 3650},
                {"traffick_air", 18000},
                {"ugc_global_registration", 128},
                {"vehicle_gen_controller", 1828}
            };
            boost::to_lower(inputStr);
            switch (yscScriptTexterIndex)
            {
            case eYscScriptTexterIndex::YSCSCRIPTTEXTER_LOAD:
                Game::RequestScript(inputStr.c_str(), vYscStackSizes.count(inputStr) ? vYscStackSizes.at(inputStr) : 14000);
                break;
            case eYscScriptTexterIndex::YSCSCRIPTTEXTER_UNLOAD:
                TERMINATE_ALL_SCRIPTS_WITH_THIS_NAME(inputStr.c_str());
                break;
            }
        }
    }

    if (blackoutOff)
    {
        SET_ARTIFICIAL_LIGHTS_STATE(FALSE);
        SET_ARTIFICIAL_VEHICLE_LIGHTS_STATE(TRUE);
    }

    if (miscPedPopulationOff)
    {
        SET_PED_POPULATION_BUDGET(1);
        return;
    }
    if (miscVehiclePopulationOff)
    {
        SET_VEHICLE_POPULATION_BUDGET(1);
        return;
    }
}

void Timecycles()
{
    bool timecyclesReset = false;
    bool timecyclesInput = false;
    bool heatVisionOn = false;
    bool nightVisionOn = false;
    bool nightVisionOff = false;
    bool strengthPlus = false;
    bool strengthMinus = false;

    AddTitle(Language::Translate("menu.misc.timecycles.title"));
    AddLocal(Language::Translate("menu.misc.timecycles.option.heat_vision.label"), GET_USINGSEETHROUGH(), heatVisionOn, heatVisionOn);
    AddToggle(Language::Translate("menu.misc.timecycles.option.heat_vision_on_aim.label"), hvSnipers);
    AddOptionDescription(Language::Translate("menu.misc.timecycles.option.heat_vision_on_aim.description"));
    AddToggle(Language::Translate("menu.misc.timecycles.option.night_vision.label"), bitNightVision, nightVisionOn, nightVisionOff);

    AddBreak(Language::Translate("menu.misc.timecycles.section.main.label"));
    AddNumber(Language::Translate("menu.misc.timecycles.option.strength.label"), currentTimecycleStrength, 2, null, strengthPlus, strengthMinus);
    AddOptionDescription(Language::Translate("menu.misc.timecycles.option.strength.description"));
    AddOption(Language::Translate("menu.misc.timecycles.option.reset.label"), timecyclesReset);

    for (auto& i : TimecycleModification::vTimecycles)
    {
        bool bModPressed = false;
        AddOption(i.second, bModPressed);
        if (bModPressed)
        {
            TimecycleModification::SetMod(i.first);
            TimecycleModification::SetModStrength(currentTimecycleStrength);
        }
    }

    AddBreak(Language::Translate("menu.misc.timecycles.section.custom.label"));
    AddOption(Language::Translate("menu.misc.timecycles.option.input_custom.label"), timecyclesInput);
    AddOptionDescription(Language::Translate("menu.misc.timecycles.option.input_custom.description"));

    if (heatVisionOn)
    {
        SET_SEETHROUGH(GET_USINGSEETHROUGH() ? FALSE : TRUE);
        return;
    }

    if (nightVisionOn)
    {
        SET_NIGHTVISION(TRUE);
        return;
    }
    if (nightVisionOff)
    {
        SET_NIGHTVISION(FALSE);
        return;
    }


    if (timecyclesReset)
    {
        TimecycleModification::ClearMod();
        currentTimecycleStrength = 0.9f;
        return;
    }

    if (timecyclesInput)
    {
        std::string inputStr = Game::InputBox("DEFAULT", 28U);
        if (inputStr.length())
        {
            SET_TIMECYCLE_MODIFIER(inputStr.c_str());
            SET_TIMECYCLE_MODIFIER_STRENGTH(currentTimecycleStrength);
        }
        return;
    }

    if (strengthPlus)
    {
        if (currentTimecycleStrength < 3.0f)
        {
            currentTimecycleStrength += 0.02f;
        }
        TimecycleModification::SetModStrength(currentTimecycleStrength);
        return;
    }
    if (strengthMinus)
    {
        if (currentTimecycleStrength > 0.0f)
        {
            currentTimecycleStrength -= 0.02f;
        }
        TimecycleModification::SetModStrength(currentTimecycleStrength);
        return;
    }
}

void ClearAreaSub()
{
    bool clearAreaVehicles = false;
    bool clearAreaPeds = false;
    bool clearAreaObjects = false;
    bool clearAreaAll = false;
    bool clearAreaRadiusPlus = false;
    bool clearAreaRadiusMinus = false;
    bool clearAreaRadiusInput = false;

    AddTitle(Language::Translate("menu.misc.clear_area.title"));

    AddNumber(Language::Translate("menu.misc.clear_area.option.range.label"), g_clearAreaRadius, 2, clearAreaRadiusInput, clearAreaRadiusPlus, clearAreaRadiusMinus);
    AddOptionDescription(Language::Translate("menu.misc.clear_area.option.range.description"));
    if (Menu::IsLastDrawnOptionSelected())
    {
        sub::Spooner::EntityManagement::DrawRadiusDisplayingMarker(GET_ENTITY_COORDS(g_activePedHandle, 1), g_clearAreaRadius);
    }

    AddOption(Language::Translate("menu.misc.clear_area.option.vehicles.label"), clearAreaVehicles);
    AddOption(Language::Translate("menu.misc.clear_area.option.peds.label"), clearAreaPeds);
    AddOption(Language::Translate("menu.misc.clear_area.option.objects.label"), clearAreaObjects);
    AddOption(Language::Translate("menu.misc.clear_area.option.all.label"), clearAreaAll);

    if (clearAreaVehicles)
    {
        ClearAreaOfEntities(EntityType::VEHICLE, GET_ENTITY_COORDS(g_activePedHandle, 1), g_clearAreaRadius, {GET_VEHICLE_PED_IS_IN(g_activePedHandle, 0)});
        return;
    }
    if (clearAreaPeds)
    {
        ClearAreaOfEntities(EntityType::PED, GET_ENTITY_COORDS(g_activePedHandle, 1), g_clearAreaRadius, {g_activePedHandle});
        return;
    }
    if (clearAreaObjects)
    {
        ClearAreaOfEntities(EntityType::PROP, GET_ENTITY_COORDS(g_activePedHandle, 1), g_clearAreaRadius, {});
        return;
    }
    if (clearAreaAll)
    {
        ClearAreaOfEntities(EntityType::ALL, GET_ENTITY_COORDS(g_activePedHandle, 1), g_clearAreaRadius, {g_activePedHandle, GET_VEHICLE_PED_IS_IN(g_activePedHandle, 0)});
        return;
    }

    if (clearAreaRadiusInput)
    {
        std::string inputStr = Game::InputBox("", 6U, "", std::to_string(g_clearAreaRadius).substr(0, 5));
        if (inputStr.length() > 0)
        {
            try
            {
                g_clearAreaRadius = stof(inputStr);
            }
            catch (...)
            {
                Game::Print::PrintErrorInvalidInput(inputStr);
                addlog(ige::LogType::LOG_ERROR, "Invalid input radius entered: " + inputStr);
            }
        }
    }
    if (clearAreaRadiusPlus)
    {
        if (g_clearAreaRadius < FLT_MAX)
        {
            g_clearAreaRadius += 0.5f;
            return;
        }
    }
    if (clearAreaRadiusMinus)
    {
        if (g_clearAreaRadius > 0.0f)
        {
            g_clearAreaRadius -= 0.5f;
            return;
        }
    }
}

void DrawToolsMenu()
{
    Vector2 startPos;
    Vector2 sizePos;
    Vector2 Pos;

    for (;;)
    {
        WAIT(0);
        SET_MOUSE_CURSOR_THIS_FRAME();
        DISABLE_ALL_CONTROL_ACTIONS(1);

        Pos = MouseSupport::MousePosition();

        if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_CURSOR_ACCEPT))
        {
            startPos = Pos;
        }
        if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_CURSOR_ACCEPT))
        {
            sizePos.x = (Pos.x - startPos.x);
            sizePos.y = (Pos.y - startPos.y);
        }

        SET_SCRIPT_GFX_ALIGN(76, 84);
        SET_SCRIPT_GFX_ALIGN_PARAMS(-0.05f, -0.05f, 0.0f, 0.0f);
        DRAW_RECT((startPos.x + sizePos.x) / 2, (startPos.y + sizePos.y) / 2, sizePos.x, sizePos.y, 107, 0, 107, 225, 0);


        DRAW_SPRITE("CommonMenu", "Gradient_Bgd", 0.90, 0.14, 0.15, 0.15, 0, 255, 255, 255, 210, false, 0);
        Game::Print::SetupDraw(7, Vector2(0.4, 0.4), true, false, false);
        Game::Print::drawstring("Details", 0.90, 0.0675);

        char str[30];

        Game::Print::SetupDraw(0, Vector2(0, 0.33), true, false, false);
        sprintf_s(str, "X - %f", startPos.x + sizePos.x);
        Game::Print::drawstring(str, 0.90, 0.0975);

        Game::Print::SetupDraw(0, Vector2(0, 0.33), true, false, false);
        sprintf_s(str, "Y - %f", startPos.y + sizePos.y);
        Game::Print::drawstring(str, 0.90, 0.1275);

        Game::Print::SetupDraw(0, Vector2(0, 0.33), true, false, false);
        sprintf_s(str, "SizeX - %f", sizePos.x);
        Game::Print::drawstring(str, 0.90, 0.1575);

        Game::Print::SetupDraw(0, Vector2(0, 0.33), true, false, false);
        sprintf_s(str, "SizeY - %f", sizePos.y);
        Game::Print::drawstring(str, 0.90, 0.1875);

        if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_FRONTEND_CANCEL))
        {
            break;
        }
    }
}

void RadioMenu()
{
    GTAped ped = g_activePedHandle;
    GTAvehicle vehicle = ped.CurrentVehicle();
    bool mobileRadioOn = false;
    bool mobileRadioOff = false;
    bool radioForward = false;
    auto& frozenStation = g_frozenRadioStation;

    AddTitle(Language::Translate("menu.misc.radio.title"));

    UINT8 vStationIds[] = {255U, 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U, 20U};

    if (ped.Equals(Game::PlayerPed())) // if this is our ped
    {
        AddLocal(Language::Translate("menu.misc.radio.option.mobile.label"), IS_MOBILE_PHONE_RADIO_ACTIVE(), mobileRadioOn, mobileRadioOff);
        AddOption(Language::Translate("menu.misc.radio.option.skip_track.label"), radioForward);
        AddBreak(Language::Translate("menu.misc.radio.section.stations.label"));

        for (UINT8 i : vStationIds)
        {
            bool stationPressed = false;
            AddTickol(GET_RADIO_STATION_NAME(i), GET_PLAYER_RADIO_STATION_INDEX() == i, stationPressed, null, frozenStation == i ? TICKOL::TICK2 : TICKOL::TICK, TICKOL::NONE, true);
            if (stationPressed)
            {
                SET_RADIO_TO_STATION_INDEX(i);
                if (ped.IsInVehicle() && vehicle.Exists())
                {
                    vehicle.RequestControl();
                    SET_VEHICLE_RADIO_ENABLED(vehicle.Handle(), true);
                    SET_VEH_RADIO_STATION(vehicle.Handle(), GET_RADIO_STATION_NAME(i));
                }
            }
            if (Menu::IsLastDrawnOptionSelected())
            {
                bool bIsCurrentlyFrozen = frozenStation == i;
                Keybinds::AddBindIB(
                    "menu_action",
                    !bIsCurrentlyFrozen,
                    Language::Translate("menu.misc.radio.option.freeze_station.label").c_str(),
                    Language::Translate("menu.misc.radio.option.unfreeze_station.label").c_str()
                );
                if (Keybinds::WasPressedThisFrame("menu_action"))
                {
                    if (bIsCurrentlyFrozen)
                    {
                        frozenStation = -1;
                    }
                    else
                    {
                        frozenStation = i;
                    }
                }
            }
        }
    }
    else // if this isn't our ped
    {
        for (UINT8 i : vStationIds)
        {
            bool stationPressed = false;
            AddOption(GET_RADIO_STATION_NAME(i), stationPressed, nullFunc, -1, false, true);
            if (stationPressed)
            {
                if (vehicle.Exists())
                {
                    vehicle.RequestControl();
                    SET_VEHICLE_RADIO_ENABLED(vehicle.Handle(), true);
                    SET_VEH_RADIO_STATION(vehicle.Handle(), GET_RADIO_STATION_NAME(i));
                }
            }
        }
    }

    if (mobileRadioOn)
    {
        SET_FRONTEND_RADIO_ACTIVE(true);
        SET_MOBILE_RADIO_ENABLED_DURING_GAMEPLAY(true);
        SET_MOBILE_PHONE_RADIO_STATE(1);
        PLAY_SOUND_FROM_ENTITY(-1, "Radio_On", g_activePedHandle, "TAXI_SOUNDS", 0, 0);
    }
    if (mobileRadioOff)
    {
        SET_MOBILE_RADIO_ENABLED_DURING_GAMEPLAY(false);
        SET_MOBILE_PHONE_RADIO_STATE(0);
        PLAY_SOUND_FROM_ENTITY(-1, "Radio_Off", g_activePedHandle, "TAXI_SOUNDS", 0, 0);
    }

    if (radioForward)
    {
        SKIP_RADIO_FORWARD();
        Game::Print::ShowNotification(oss_ << Game::GetGXTEntry(GET_PLAYER_RADIO_STATION_NAME()) << " - next track");
    }
}

namespace WaterHack
{
class WaterHack final
{
  private:
    bool bEnabled = false;

  public:
    WaterHack()
    {
    }

    bool& Enabled()
    {
        return bEnabled;
    }

    float radius = 0.0f;
    float height = 5.0f;

    void Tick()
    {
        if (bEnabled)
        {
            DoWaterHack();
        }
    }

    inline void DoWaterHack()
    {
        GTAped ped = PLAYER_PED_ID();
        const Vector3& centre = ped.GetPosition();

        std::vector<Vector3> points;
        centre.PointsOnCircle(points, this->radius, this->radius < 10.0f ? 60.0f : 13.0f, 3.5f, true);

        for (auto& current : points)
        {
            WATER::MODIFY_WATER(current.x, current.y, current.z, this->height);
        }
    }
};

WaterHack g_waterHack;

void Tick()
{
    g_waterHack.Tick();
}

void Sub_WaterHack()
{
    float& radius = g_waterHack.radius;
    float& height = g_waterHack.height;

    bool radiusPlus = false;
    bool radiusMinus = false;
    bool heightPlus = false;
    bool heightMinus = false;
    bool radiusInput = false;
    bool heightInput = false;

    AddTitle(Language::Translate("menu.misc.water_hack.title"));
    AddToggle(Language::Translate("menu.misc.water_hack.option.toggle.label"), g_waterHack.Enabled());
    AddOptionDescription(Language::Translate("menu.misc.water_hack.option.toggle.description"));
    AddNumber(Language::Translate("menu.misc.water_hack.option.radius.label"), radius, 1, radiusInput, radiusPlus, radiusMinus);
    AddNumber(Language::Translate("menu.misc.water_hack.option.height.label"), height, 1, heightInput, heightPlus, heightMinus);


    if (radiusPlus)
    {
        if (radius < 650.0f)
        {
            radius += 3.0f;
        }
        return;
    }
    if (radiusMinus)
    {
        if (radius > 0.0f)
        {
            radius -= 3.0f;
        }
        return;
    }
    if (radiusInput)
    {
        std::string inputStr = Game::InputBox("", 10U, "", std::to_string(radius).substr(0, 9));
        if (inputStr.length() > 0)
        {
            float oldVal = radius;
            try
            {
                radius = stof(inputStr);
                if (radius > 650.0f)
                {
                    radius = 650.0f;
                }
                else if (radius < 0.0f)
                {
                    radius = 0.0f;
                }
            }
            catch (...)
            {
                radius = oldVal;
                addlog(ige::LogType::LOG_ERROR, "Invalid radius entered: " + std::to_string(radius) + ", returning to oldVal: " + std::to_string(oldVal));
            }
        }
        return;
    }

    if (heightPlus)
    {
        if (height < 800.0f)
        {
            height += 0.1f;
        }
        return;
    }
    if (heightMinus)
    {
        if (height > -800.0f)
        {
            height -= 0.1f;
        }
        return;
    }
    if (heightInput)
    {
        std::string inputStr = Game::InputBox("", 10U, "", std::to_string(height).substr(0, 9));
        if (inputStr.length() > 0)
        {
            try
            {
                height = stof(inputStr);
                height = (std::min)(800.0f, (std::max)(-800.0f, height));
            }
            catch (...)
            {
                addlog(ige::LogType::LOG_ERROR, "Invalid height entered: " + std::to_string(height));
            }
        }
        return;
    }
}
} // namespace WaterHack

namespace TVChannelStuff
{
bool loopBasicTV = false;

std::string currentTvChannelLabel;
std::map<std::string, std::string> tvPlaylists{
    {"CNT", "PL_STD_CNT"},
    {"Weazel News", "PL_STD_WZL"},
    {"Weazel News Transition", "PL_MP_WEAZEL"},
    {"Lotto Adverts", "PL_LO_CNT"},
    {"Piswasser Adverts", "PL_LO_WZL"},
    {"Righteous Slaughter", "PL_LO_RS"},
    {"Righteous Slaughter Cutscene", "PL_LO_RS_CUTSCENE"},
    {"Workout", "PL_SP_WORKOUT"},
    {"Life Invader", "PL_SP_INV"},
    {"Life Invader Explosion", "PL_SP_INV_EXP"},
    {"Weazel Adverts", "PL_SP_PLSH1_INTRO"},
    {"Fame or Shame Episode 1", "PL_LES1_FAME_OR_SHAME"},
    {"Fame or Shame Episode 2", "PL_STD_WZL_FOS_EP2"},
    {"Capolavoro", "PL_CINEMA_ARTHOUSE"},
    {"The Loneliest Robot", "PL_CINEMA_CARTOON"},
    {"Meltdown", "PL_CINEMA_ACTION"},
    {"Cinema (No Meltdown)", "PL_CINEMA_MULTIPLAYER_NO_MELTDOWN"},
    {"Jack Howitzer", "PL_WEB_HOWITZER"},
    {"Kung Fu Rainbow LazerForce", "PL_WEB_KFLF"},
    {"Republican Space Rangers", "PL_WEB_RANGERS"},
    {"CCTV", "PL_MP_CCTV"}
};

void DrawTvWhereItsSupposedToBe()
{
    Vector2 scale = {0.2f, 0.2f};
    Vector2 pos = {menuPos.x > 0.45f ? 0.0f + scale.x / 2 : 1.0f - scale.x / 2, 0.1f + scale.y / 2};

    GRAPHICS::SET_TV_AUDIO_FRONTEND(true);
    GRAPHICS::DRAW_TV_CHANNEL(pos.x, pos.y, scale.x, scale.y, 0.0f, 255, 255, 255, 250);
}

void TVMenu()
{
    float minVolume = -36.0f;
    float maxVolume = 0.0f;
    auto currentChannel = GRAPHICS::GET_TV_CHANNEL();
    auto currentVolume = GRAPHICS::GET_TV_VOLUME();

    AddTitle(Language::Translate("menu.misc.tv.title"));
    AddToggle(Language::Translate("menu.misc.tv.option.toggle_player.label"), loopBasicTV);

    bool volumePlus = false;
    bool volumeMinus = false;

    AddNumber(Language::Translate("menu.misc.tv.option.volume.label"), currentVolume + (maxVolume - minVolume), 0, null, volumePlus, volumeMinus);
    if (volumePlus)
    {
        if (currentVolume < maxVolume)
        {
            currentVolume = round(currentVolume + 1.0f);
            SET_TV_VOLUME(currentVolume);
        }
    }
    if (volumeMinus)
    {
        if (currentVolume > minVolume)
        {
            currentVolume = round(currentVolume - 1.0f);
            SET_TV_VOLUME(currentVolume);
        }
    }

    AddBreak(Language::Translate("menu.misc.tv.section.playlists.label"));
    for (auto& pl : tvPlaylists)
    {
        bool playlistPressed = false;
        AddOption(pl.first, playlistPressed);
        if (playlistPressed)
        {
            GRAPHICS::IS_TVSHOW_CURRENTLY_PLAYING(80996397); //0x0AD973CA1E077B60
            GRAPHICS::SET_TV_CHANNEL(-1);
            GRAPHICS::SET_TV_CHANNEL_PLAYLIST(0, pl.second.c_str(), 1);
            GRAPHICS::SET_TV_CHANNEL(0);
        }
    }
}
} // namespace TVChannelStuff

namespace HudOptions
{
bool revealMinimap = false;

void HudOptionsMenu()
{
    AddTitle(Language::Translate("menu.misc.hud.title"));

    bool revealMinimapToggle = false;
    AddToggle(Language::Translate("menu.misc.hud.option.reveal_minimap.label"), revealMinimap, revealMinimapToggle, revealMinimapToggle);
    AddOptionDescription(Language::Translate("menu.misc.hud.option.reveal_minimap.description"));
    if (revealMinimapToggle)
    {
        SET_MINIMAP_HIDE_FOW(revealMinimap);
    }

    AddToggle(Language::Translate("menu.misc.hud.option.display_coordinates.label"), bDisplayXyzhCoords);
    AddOptionDescription(Language::Translate("menu.misc.hud.option.display_coordinates.description"));
    AddToggle(Language::Translate("menu.misc.hud.option.display_fps.label"), FPSCounter::bDisplayFps);
    AddToggle(Language::Translate("menu.misc.hud.option.hide.label"), hideHUD);
    AddToggle(Language::Translate("menu.misc.hud.option.show_full.label"), showFullHUD);
    AddOptionDescription(Language::Translate("menu.misc.hud.option.show_full.description"));

    AddBreak(Language::Translate("menu.misc.hud.section.component_colours.label"));
    for (auto& h : std::vector<std::pair<int, std::string>>{
             {HudColour::PURE_WHITE, Language::Translate("menu.misc.hud.option.map_blips.label")},
             {HudColour::WAYPOINT, Language::Translate("menu.misc.hud.option.waypoint.label")},
             {HudColour::WHITE, Language::Translate("menu.misc.hud.option.pause_menu_text.label")},
             {HudColour::PAUSE_BG, Language::Translate("menu.misc.hud.option.pause_menu_background.label")},
             {HudColour::PAUSEMAP_TINT, Language::Translate("menu.misc.hud.option.pause_menu_map_tint.label")},
             {HudColour::BLACK, Language::Translate("menu.misc.hud.option.selected_text.label")},
             {HudColour::FREEMODE, Language::Translate("menu.misc.hud.option.freemode.label")},
             {HudColour::MICHAEL, Language::Translate("menu.misc.hud.option.michael.label")},
             {HudColour::FRANKLIN, Language::Translate("menu.misc.hud.option.franklin.label")},
             {HudColour::TREVOR, Language::Translate("menu.misc.hud.option.trevor.label")}
         })
    {
        bool bCompSelected = false;
        AddOption(h.second, bCompSelected, nullFunc, SUB::MSPAINTS_RGB);
        if (bCompSelected)
        {
            SetSelectedHudColour(h.first);
            bitMSPaintsRGBMode = 10;
        }
    }

    AddBreak(Language::Translate("menu.misc.hud.section.all_colours.label"));
    for (int i = 0; i < HudColour::vHudColours.size(); i++)
    {
        bool bCompSelected = false;
        AddOption(HudColour::vHudColours[i], bCompSelected, nullFunc, SUB::MSPAINTS_RGB);
        if (bCompSelected)
        {
            SetSelectedHudColour(i);
            bitMSPaintsRGBMode = 10;
        }
    }
}
} // namespace HudOptions

namespace GameCamOptions
{
float shakeAmplitude = 1.0f;
INT8 shakeID = -1;

void GameCamOptionsMenu()
{
    AddTitle(Language::Translate("menu.misc.game_camera.title"));

    auto& shakeNames = cameraShakeNames;

    bool shakeTypePlus = false;
    bool shakeTypeMinus = false;
    AddTexter(
        Language::Translate("menu.misc.game_camera.option.shake_type.label"),
        shakeID < 0 ? 0 : shakeID,
        shakeID < 0 ? std::vector<std::string>{Language::Translate("menu.misc.option.game_camera.value.none")} : shakeNames,
        null,
        shakeTypePlus,
        shakeTypeMinus
    );
    AddOptionDescription(Language::Translate("menu.misc.game_camera.option.shake_type.description"));
    if (shakeTypePlus)
    {
        if (shakeID < (INT8)(shakeNames.size() - 1))
        {
            shakeID++;
            GameplayCamera::Shake(static_cast<CameraShake>(shakeID), shakeAmplitude);
        }
    }
    if (shakeTypeMinus)
    {
        if (shakeID > -1)
        {
            shakeID--;
            if (shakeID < 0)
            {
                GameplayCamera::StopShaking(true);
            }
            else
            {
                GameplayCamera::Shake(static_cast<CameraShake>(shakeID), shakeAmplitude);
            }
        }
    }

    bool shakeAmplitudePlus = false;
    bool shakeAmplitudeMinus = false;
    AddNumber(Language::Translate("menu.misc.game_camera.option.shake_amplitude.label"), shakeAmplitude, 2, null, shakeAmplitudePlus, shakeAmplitudeMinus);
    AddOptionDescription(Language::Translate("menu.misc.game_camera.option.shake_amplitude.description"));
    if (shakeAmplitudePlus)
    {
        shakeAmplitude += 0.05f;
        if (!GameplayCamera::IsShaking())
        {
            GameplayCamera::Shake(static_cast<CameraShake>(shakeID), shakeAmplitude);
        }
        else
        {
            GameplayCamera::SetShakeAmplitude(shakeAmplitude);
        }
    }
    if (shakeAmplitudeMinus)
    {
        shakeAmplitude -= 0.05f;
        if (!GameplayCamera::IsShaking())
        {
            GameplayCamera::Shake(static_cast<CameraShake>(shakeID), shakeAmplitude);
        }
        else
        {
            GameplayCamera::SetShakeAmplitude(shakeAmplitude);
        }
    }
}
} // namespace GameCamOptions

namespace FreeCamSettings
{
namespace
{
void ApplyFovToActiveCameras(float fov)
{
    Camera freeCam = FreeCamMode::GetCamera();
    if (freeCam.Exists())
        freeCam.SetFieldOfView(fov);

    Camera& spoonerCam = Spooner::SpoonerCamera::camera;
    if (spoonerCam.Exists())
        spoonerCam.SetFieldOfView(fov);
}
} // namespace

void FreeCamSettingsMenu()
{
    namespace Cfg = MenuConfig::FreeCam;

    bool freecamOn = false;
    bool freecamOff = false;

    AddTitle(Language::Translate("menu.misc.free_cam.title"));
    AddToggle(Language::Translate("menu.misc.free_cam.option.toggle.label"), FreeCamMode::bEnabled, freecamOn, freecamOff);

    AddBreak(Language::Translate("menu.misc.free_cam.section.shared.label"));
    bool changed = false;
    changed |= AddNumberStepper(Language::Translate("menu.misc.free_cam.option.movement_speed.label"), Cfg::defaultSpeed, 2, Cfg::speedAdjustStep, Cfg::minSpeed, Cfg::maxSpeed);
    AddOptionDescription(Language::Translate("menu.misc.free_cam.option.movement_speed.description"));
    changed |= AddNumberStepper(Language::Translate("menu.misc.free_cam.option.slow_speed.label"), Cfg::defaultSlowSpeed, 2, Cfg::speedAdjustStep, Cfg::minSpeed, Cfg::maxSpeed);
    AddOptionDescription(Language::Translate("menu.misc.free_cam.option.slow_speed.description"));
    if (AddNumberStepper(Language::Translate("menu.misc.free_cam.option.fov.label"), Cfg::defaultFov, 1, Cfg::fovAdjustStep, Cfg::minFov, Cfg::maxFov))
    {
        changed = true;
        ApplyFovToActiveCameras(Cfg::defaultFov);
    }
    AddOptionDescription(Language::Translate("menu.misc.free_cam.option.fov.description"));
    changed |= AddNumberStepper(Language::Translate("menu.misc.free_cam.option.rotation_sensitivity_mouse.label"), Cfg::rotationSensitivityMouse, 1, 0.5, 0.5, 50.0);
    changed |= AddNumberStepper(Language::Translate("menu.misc.free_cam.option.rotation_sensitivity_gamepad.label"), Cfg::rotationSensitivityGamepad, 1, 0.1, 0.1, 10.0);
    if (changed)
        MenuConfig::RequestSave();

    bool resetToDefaults = false;
    AddOption(Language::Translate("menu.misc.free_cam.option.reset.label"), resetToDefaults);
    AddOptionDescription(Language::Translate("menu.misc.free_cam.option.reset.description"));
    if (resetToDefaults)
    {
        Cfg::ResetToDefaults();
        ApplyFovToActiveCameras(Cfg::defaultFov);
        Game::Print::ShowNotification(Language::Translate("message.misc.free_cam.reset.title"));
    }

    if (freecamOn)
        Game::Print::ShowNotification(Language::Format("message.misc.free_cam.toggle.title", {{"key", Keybinds::GetGlyph("no_clip")}}));

    if (freecamOff)
        FreeCamMode::Stop();
}
} // namespace FreeCamSettings
} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(TIMECYCLES, sub::Timecycles)
REGISTER_SUBMENU(CLEARAREA, sub::ClearAreaSub)
REGISTER_SUBMENU(WATERHACK, sub::WaterHack::Sub_WaterHack)
REGISTER_SUBMENU(MISCOPS, sub::MiscOps)
REGISTER_SUBMENU(TVCHANNELSTUFF_TV, sub::TVChannelStuff::TVMenu)
REGISTER_SUBMENU(HUDOPTIONS, sub::HudOptions::HudOptionsMenu)
REGISTER_SUBMENU(GAMECAMOPTIONS, sub::GameCamOptions::GameCamOptionsMenu)
REGISTER_SUBMENU(FREECAMSETTINGS, sub::FreeCamSettings::FreeCamSettingsMenu)
REGISTER_SUBMENU(RADIOSUB, sub::RadioMenu)
