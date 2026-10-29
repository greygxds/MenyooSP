/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "VehicleOptions.h"
#include "VehicleModShop.h"
#include "..\Menu\Keybinds.h"
#include "..\Menu\Language.h"
#include "..\Util\FileLogger.h"

namespace sub
{
// Vehicle - ped task
void TaskRappel(GTAped ped, GTAvehicle vehicle)
{
    ped.RequestControl();
    ped.Task().ClearAll();
    ped.SetIntoVehicle(vehicle, VehicleSeat::SEAT_LEFTREAR);

    GTAped newPed;
    if (vehicle.IsSeatFree(VehicleSeat::SEAT_DRIVER))
    {
        newPed = World::CreatePedInsideVehicle(PedHash::PrologueDriver, vehicle, VehicleSeat::SEAT_DRIVER);
    }

    WAIT(100);

    if (newPed.Exists())
    {
        const Vector3& vehPos = vehicle.GetPosition();
        newPed.SetBlockPermanentEvent(true);
        TASK_HELI_MISSION(newPed.Handle(), vehicle.Handle(), 0, 0, vehPos.x, vehPos.y, vehPos.z, 4, 0.0f, 50.0f, -1.0f, 10000, 100, -1082130432, 0);
        newPed.SetAlwaysKeepTask(true);
    }

    vehicle.SetVelocity(Vector3::Zero());

    ped.RequestControl();
    TASK_RAPPEL_FROM_HELI(ped.Handle(), 1); // 1092616192 0x41200000

    WAIT(100);
    if (newPed.Exists())
    {
        newPed.NoLongerNeeded(); // Too soon, maybe?
    }
}

void VehicleMenu()
{
    bool setVehiclePed = false;
    bool fixCar = false;
    bool toggleCargobobMagnet = false;
    bool toggleBoatAnchor = false;
    bool funnyVehicleFreezeOff = false;
    bool disableSirenOff = false;
    bool vehicleMenuModShop = false;
    bool vehicleMenuAlphaLevel = false;
    bool rappelHeli = false;
    bool teleportClosestCar = false;
    bool damageAndDefensePlus = false;
    bool damageAndDefenseMinus = false;
    bool damageAndDefenseCustom = false;
    bool invincibilityOff = false;
    bool invisible = false;
    bool slippyTires = false;
    bool vehicleMenuEngine = false;
    bool interiorLight = false;
    bool multiplyLights = false;
    bool boostOn = false;
    bool hydraulicsOn = false;
    bool doorLocksOn = false;
    bool doorLocksOff = false;
    bool noGravityOn = false;
    bool noGravityOff = false;
    bool speedoOn = false;
    bool collisionsOn = false;
    bool collisionsOff = false;
    bool freezeCarOn = false;
    bool freezeCarOff = false;
    bool deleteCar = false;

    g_activePedHandle = PLAYER_PED_ID();
    g_activePlayerId = PLAYER_ID();

    bool amIOnline = NETWORK_IS_IN_SESSION() != 0;
    GTAped myPed = g_activePedHandle;
    GTAplayer myPlayer = g_activePlayerId;
    GTAvehicle myVehicle = g_myVeh;
    bool myPedIsInVehicle = myPed.IsInVehicle();
    const Model& myVehicleModel = myVehicle.Model();

    static int fixCarTexterValue = 0;
    std::vector<std::string> fixCarTexter{
        Language::Translate("vehicle.options.repair.full"),
        Language::Translate("vehicle.options.repair.keepDirt"),
        Language::Translate("vehicle.options.repair.keepWindowsOpen"),
        Language::Translate("vehicle.options.repair.keepWindowsOpenWithDirt")
    };
    auto& fixCarTexterVal = fixCarTexterValue;
    bool fixCarPlus = false;
    bool fixCarMinus = false;

    AddTitle(Language::Translate("vehicle.options.title"));
    if (myPedIsInVehicle)
    {
        AddTexter("CMOD_MOD_MNT", fixCarTexterVal, fixCarTexter, fixCar, fixCarPlus, fixCarMinus, true); // Fix & Wash
        AddOptionDescription(Language::Translate("vehicle.options.repair.description"));
    }
    AddOption(Language::Translate("vehicle.options.vehicleSpawner.label"), funnyVehicleFreezeOff, nullFunc, SUB::SPAWNVEHICLE);
    AddOption(Language::Translate("vehicle.options.menyooCustoms.label"), vehicleMenuModShop, nullFunc, -1, true);
    if (myVehicleModel.IsCargobob())
    {
        AddLocal(Language::Translate("vehicle.options.cargobobMagnet.label"), myVehicle.IsCargobobHookActive(CargobobHook::Magnet), toggleCargobobMagnet, toggleCargobobMagnet);
    }
    if (DOES_VEHICLE_ALLOW_RAPPEL(g_myVeh))
    {
        AddOption(Language::Translate("vehicle.options.rappelFromHelicopter.label"), rappelHeli);
    }
    if (myVehicle.GetHasSiren())
    {
        AddToggle(Language::Translate("vehicle.options.disableVehicleSiren.label"), vehicleDisableSiren, null, disableSirenOff);
    }
    if (myVehicleModel.IsBoat())
    {
        AddLocal(Language::Translate("vehicle.options.anchorBoat.label"), myVehicle.IsBoatAnchored(), toggleBoatAnchor, toggleBoatAnchor);
    }
    AddOption(Language::Translate("vehicle.options.teleportIntoClosestVehicle.label"), teleportClosestCar);
    AddOption(Language::Translate("vehicle.options.vehicleMultipliers.label"), null, nullFunc, SUB::VEHICLEMULTIPLIERS);
    AddOption(Language::Translate("vehicle.options.speedometers.label"), null, nullFunc, SUB::VEHICLESPEEDOS);
    AddOption(Language::Translate("vehicle.options.multiPlatformNeons.label"), null, nullFunc, SUB::MULTIPLATNEONS);
    AddOptionDescription(Language::Translate("vehicle.options.multiPlatformNeons.description"));
    AddOption(Language::Translate("vehicle.options.pvOptions.label"), null, nullFunc, SUB::PVOPS);
    AddOptionDescription(Language::Translate("vehicle.options.pvOptions.description"));
    AddOption(Language::Translate("vehicle.options.autoDrive.label"), null, nullFunc, SUB::AUTODRIVESUB);
    AddOptionDescription(Language::Translate("vehicle.options.autoDrive.description"));
    AddOption(Language::Translate("vehicle.options.opacityLocal.label"), vehicleMenuAlphaLevel, nullFunc, -1, true);
    AddOptionDescription(Language::Translate("vehicle.options.opacityLocal.description"));
    AddNumber(Language::Translate("vehicle.options.damageDefense.label"), vehicleDamageAndDefense, 2, damageAndDefenseCustom, damageAndDefensePlus, damageAndDefenseMinus);
    AddOptionDescription(Language::Translate("vehicle.options.damageDefense.description"));
    AddToggle(Language::Translate("vehicle.options.invincibilityLooped.label"), vehicleInvincibility, null, invincibilityOff);
    AddOptionDescription(Language::Translate("vehicle.options.invincibilityLooped.description"));
    AddLocal(Language::Translate("vehicle.options.invisibility.label"), !myVehicle.IsVisible(), vehicleInvisibility, invisible);
    AddToggle(Language::Translate("vehicle.options.autoRepair.label"), vehicleFixLoop);
    AddToggle(Language::Translate("vehicle.options.autoFlip.label"), vehicleFlipLoop);
    AddOptionDescription(Language::Translate("vehicle.options.autoFlip.description"));
    AddToggle(Language::Translate("vehicle.options.keepEngineLightsOn.label"), selfEngineOn);
    AddOptionDescription(Language::Translate("vehicle.options.keepEngineLightsOn.description"));
    AddTickol(Language::Translate("vehicle.options.killEngine.label"), (GET_VEHICLE_ENGINE_HEALTH(g_myVeh) < 0.0f), vehicleMenuEngine, vehicleMenuEngine, TICKOL::PERCENTAGESTICKER, TICKOL::NONE);
    AddOptionDescription(Language::Translate("vehicle.options.killEngine.description"));
    AddLocal(Language::Translate("vehicle.options.slidyTyres.label"), bitVehicleSlippyTires, slippyTires, slippyTires);
    AddOptionDescription(Language::Translate("vehicle.options.slidyTyres.description"));
    AddLocal(Language::Translate("vehicle.options.cruiseControl.label"), VehicleCruise::g_vehicleCruise.Enabled(), VehicleCruise::ToggleOnOff, VehicleCruise::ToggleOnOff);
    AddOptionDescription(Language::Translate("vehicle.options.cruiseControl.description"));
    AddToggle(Language::Translate("vehicle.options.vehicleBoost.label"), raceBoost, boostOn);
    AddOptionDescription(Language::Translate("vehicle.options.vehicleBoost.description"));
    AddToggle(Language::Translate("vehicle.options.infiniteNativeBoost.label"), unlimitedVehicleBoost);
    AddOptionDescription(Language::Translate("vehicle.options.infiniteNativeBoost.description"));
    AddToggle(Language::Translate("vehicle.options.hydraulics.label"), carHydraulics, hydraulicsOn);
    AddOptionDescription(Language::Translate("vehicle.options.hydraulics.description"));
    AddTickol(Language::Translate("vehicle.options.doorLocks.label"), (GET_VEHICLE_DOOR_LOCK_STATUS(g_myVeh) == 2), doorLocksOn, doorLocksOff, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddLocal(Language::Translate("vehicle.options.noGravity.label"), bitVehicleGravity, noGravityOn, noGravityOff);
    AddLocal(Language::Translate("vehicle.options.freezeVehicle.label"), bitFreezeVehicle, freezeCarOn, freezeCarOff);
    AddLocal(Language::Translate("vehicle.options.collision.label"), myVehicle.GetIsCollisionEnabled(), collisionsOn, collisionsOff);
    AddOptionDescription(Language::Translate("vehicle.options.collision.description"));
    AddOption(Language::Translate("vehicle.options.deleteVehicle.label"), deleteCar);

    if (toggleCargobobMagnet)
    {
        if (myVehicle.IsCargobobHookActive(CargobobHook::Magnet))
        {
            myVehicle.RequestControl();
            myVehicle.RetractCargobobHook();
            WAIT(50);
            myVehicle.DropCargobobHook(CargobobHook::Hook);
        }
        else
        {
            myVehicle.RequestControl();
            myVehicle.DropCargobobHook(CargobobHook::Magnet);
            myVehicle.CargoBobMagnetGrabVehicle();
        }
    }

    if (toggleBoatAnchor)
    {
        myVehicle.AnchorBoat(!myVehicle.IsBoatAnchored());
    }

    if (disableSirenOff)
    {
        SET_VEHICLE_HAS_MUTED_SIRENS(g_myVeh, FALSE);
    }

    if (collisionsOn || collisionsOff)
    {
        myVehicle.SetIsCollisionEnabled(!myVehicle.GetIsCollisionEnabled());
    }

    if (setVehiclePed)
    {
        sub::SetVehicleModShopTarget(g_myVeh);
    }
    if (vehicleMenuModShop)
    {
        if (DOES_ENTITY_EXIST(g_myVeh))
        {
            sub::SetVehicleModShopTarget(g_myVeh);
            Menu::NewSetMenu(SUB::MODSHOP);
        }
        else
        {
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        }
        return;
    }
    if (vehicleMenuAlphaLevel)
    {
        if (DOES_ENTITY_EXIST(g_myVeh))
        {
            sub::SetVehicleModShopTarget(g_myVeh);
            Menu::NewSetMenu(SUB::ENTITYALPHALEVEL);
        }
        else
        {
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        }
        return;
    }

    if (rappelHeli)
    {
        if (myPed.IsInVehicle())
        {
            TaskRappel(myPed.Handle(), myPed.CurrentVehicle());
            return;
        }
    }

    if (teleportClosestCar)
    {
        const GTAvehicle& tempVehicle = World::GetClosestVehicle(myPed.GetPosition(), FLT_MAX);
        if (tempVehicle.Exists())
        {
            myPed.SetIntoVehicle(tempVehicle, tempVehicle.FirstFreeSeat(SEAT_DRIVER));
        }
        return;
    }

    if (fixCarPlus && fixCarTexterVal < fixCarTexter.size() - 1)
    {
        fixCarTexterVal++;
    }
    else if (fixCarMinus && fixCarTexterVal > 0)
    {
        fixCarTexterVal--;
    }
    if (fixCar)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
        {
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        }
        else
        {
            std::vector<VehicleWindow> windowsToOpen;
            if (fixCarTexterVal == 1 || fixCarTexterVal == 3)
            {
                for (int i = (int)VehicleWindow::FrontLeftWindow; i < (int)VehicleWindow::Last; i++)
                {
                    if (!myVehicle.IsWindowIntact((VehicleWindow)i))
                    {
                        windowsToOpen.push_back((VehicleWindow)i);
                    }
                }
            }


            myVehicle.RequestControlOnce();
            SET_VEHICLE_FIXED(g_myVeh);
            if (fixCarTexterVal == 0 || fixCarTexterVal == 2)
            {
                SET_VEHICLE_DIRT_LEVEL(g_myVeh, 0.0f);
            }
            SET_VEHICLE_ENGINE_CAN_DEGRADE(g_myVeh, 0);
            SET_VEHICLE_ENGINE_HEALTH(g_myVeh, 1250.0f);
            SET_VEHICLE_PETROL_TANK_HEALTH(g_myVeh, 1250.0f);
            SET_VEHICLE_BODY_HEALTH(g_myVeh, 1250.0f);
            SET_VEHICLE_UNDRIVEABLE(g_myVeh, 0);
            if (!GET_IS_VEHICLE_ENGINE_RUNNING(g_myVeh))
            {
                SET_VEHICLE_ENGINE_ON(g_myVeh, 1, 1, 0);
            }

            if (fixCarTexterVal == 1 || fixCarTexterVal == 3)
            {
                for (auto& i : windowsToOpen)
                {
                    myVehicle.RollDownWindow(i);
                }
            }
        }
        return;
    }

    if (damageAndDefensePlus)
    {
        vehicleDamageAndDefense += 0.2f;
        SET_PLAYER_VEHICLE_DAMAGE_MODIFIER(PLAYER_ID(), vehicleDamageAndDefense);
        SET_PLAYER_VEHICLE_DEFENSE_MODIFIER(PLAYER_ID(), vehicleDamageAndDefense);
        return;
    }
    if (damageAndDefenseMinus)
    {
        vehicleDamageAndDefense -= 0.2f;
        SET_PLAYER_VEHICLE_DAMAGE_MODIFIER(PLAYER_ID(), vehicleDamageAndDefense);
        SET_PLAYER_VEHICLE_DEFENSE_MODIFIER(PLAYER_ID(), vehicleDamageAndDefense);
        return;
    }
    if (damageAndDefenseCustom)
    {
        std::string inputStr = Game::InputBox("", 10U);
        if (inputStr.length() > 0)
        {
            float oldVal = vehicleDamageAndDefense;
            bool noexc = true;
            try
            {
                vehicleDamageAndDefense = stof(inputStr);
            }
            catch (...)
            {
                noexc = false;
                vehicleDamageAndDefense = oldVal;
            }
            if (noexc)
            {
                SET_PLAYER_VEHICLE_DAMAGE_MODIFIER(PLAYER_ID(), vehicleDamageAndDefense);
                SET_PLAYER_VEHICLE_DEFENSE_MODIFIER(PLAYER_ID(), vehicleDamageAndDefense);
            }
        }
        return;
    }

    if (invincibilityOff)
    {
        if (IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
        {
            myVehicle.RequestControlOnce();
            SetVehicleInvincibleOff(g_myVeh);
        }
        return;
    }

    if (invisible)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        else
        {
            myVehicle.RequestControlOnce();
            myVehicle.SetVisible(true);
        }
        return;
    }

    if (slippyTires)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        else
        {
            bitVehicleSlippyTires = !bitVehicleSlippyTires;
            myVehicle.RequestControlOnce();
            SET_VEHICLE_REDUCE_GRIP(g_myVeh, bitVehicleSlippyTires);
        }
        return;
    }

    if (vehicleMenuEngine)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        else
        {
            if (GET_VEHICLE_ENGINE_HEALTH(g_myVeh) > 0.0f)
            {
                myVehicle.RequestControlOnce();
                SET_VEHICLE_ENGINE_HEALTH(g_myVeh, -1.0f);
                SET_VEHICLE_UNDRIVEABLE(g_myVeh, 1);
                Game::Print::ShowNotification(Language::Translate("vehicle.notifications.engineKilled"));
            }
            else
            {
                myVehicle.RequestControlOnce();
                SET_VEHICLE_ENGINE_HEALTH(g_myVeh, 1250.0f);
                SET_VEHICLE_UNDRIVEABLE(g_myVeh, 0);
                Game::Print::ShowNotification(Language::Translate("vehicle.notifications.engineRevived"));
            }
        }
        return;
    }

    if (hydraulicsOn)
    {
        Game::Print::PrintBottomLeft(Language::Format("vehicle.notifications.hydraulicsControls", {{"controls", Menu::usingControllerInput ? "LS/L1 + stick movement" : "LeftShift + WASD"}}));
        return;
    }

    if (doorLocksOn)
    {
        myVehicle.RequestControlOnce();
        SET_VEHICLE_DOORS_LOCKED(g_myVeh, 2);
        return;
    }
    if (doorLocksOff)
    {
        myVehicle.RequestControlOnce();
        SET_VEHICLE_DOORS_LOCKED(g_myVeh, 0);
        return;
    }

    if (noGravityOn)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        else
        {
            myVehicle.RequestControlOnce();
            SET_VEHICLE_GRAVITY(g_myVeh, 0);
            bitVehicleGravity = true;
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.gravityDisabled"));
        }
        return;
    }
    if (noGravityOff)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        else
        {
            myVehicle.RequestControlOnce();
            SET_VEHICLE_GRAVITY(g_myVeh, 1);
            bitVehicleGravity = false;
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.gravityEnabled"));
        }
        return;
    }

    if (freezeCarOn)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        else
        {
            bitFreezeVehicle = true;
            myVehicle.RequestControlOnce();
            SET_VEHICLE_FORWARD_SPEED(g_myVeh, 0.0f);
            FREEZE_ENTITY_POSITION(g_myVeh, 1);
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.freezeOn"));
        }
        return;
    }

    if (freezeCarOff)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        else
        {
            bitFreezeVehicle = false;
            myVehicle.RequestControlOnce();
            SET_VEHICLE_FORWARD_SPEED(g_myVeh, 8.0f);
            FREEZE_ENTITY_POSITION(g_myVeh, 0);
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.freezeOff"));
        }
        return;
    }

    if (deleteCar)
    {
        if (!IS_PED_IN_ANY_VEHICLE(PLAYER_PED_ID(), 0))
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        else
        {
            myVehicle.Delete(false);
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.poof"));
            *Menu::activeOptionIndex = 1;
        }
        return;
    }
}

void PVMenu()
{
    bool pvSave = false;
    bool pvTeleportIntoIt = false;
    bool pvTeleportToIt = false;
    bool pvTeleportItToYou = false;
    bool pvUnsave = false;

    GTAped myPed = PLAYER_PED_ID();
    GTAvehicle myVehicle = g_myVeh;
    GTAvehicle& pv = pvSubVehicleID;

    AddTitle(Language::Translate("vehicle.pv.title"));
    AddTickol(Language::Translate("vehicle.pv.rememberVehicle.label"), pv == myVehicle, pvSave, pvUnsave);
    AddOptionDescription(Language::Translate("vehicle.pv.rememberVehicle.description"));
    AddOption(Language::Translate("vehicle.pv.teleportIntoSeat.label"), pvTeleportIntoIt);
    AddOption(Language::Translate("vehicle.pv.teleportToVehicle.label"), pvTeleportToIt);
    AddOption(Language::Translate("vehicle.pv.teleportVehicleToSelf.label"), pvTeleportItToYou);

    if (pvUnsave)
    {
        pv.SetMissionEntity(false);
        GTAblip blip = pv.CurrentBlip();
        if (blip.Exists())
        {
            if (blip.Colour() == 61)
            {
                blip.Remove();
            }
        }
        pv = 0;
        return;
    }

    if (pvSave)
    {
        if (!myPed.IsInVehicle())
        {
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.notInVehicle"));
        }
        else
        {
            pv = myVehicle;
            pv.SetMissionEntity(true);

            GTAblip blip = pv.CurrentBlip();
            if (!blip.Exists())
            {
                blip = pv.AddBlip();
                if (blip.Exists())
                {
                    Model model = pv.Model();
                    BlipIcon::BlipIcon sprite = BlipIcon::PersonalVehicleCar;

                    if (model.IsHeli())
                    {
                        sprite = BlipIcon::EnemyHelicopter;
                    }
                    else if (model.IsPlane())
                    {
                        sprite = BlipIcon::Plane;
                    }
                    else if (model.IsBoat())
                    {
                        sprite = BlipIcon::Boat;
                    }
                    else if (model.IsBike())
                    {
                        sprite = BlipIcon::PersonalVehicleBike;
                    }
                    else if (model.hash == VEHICLE_RHINO)
                    {
                        sprite = BlipIcon::Tank; // Rhino
                    }

                    blip.SetIcon(sprite);
                    blip.SetScale(0.7f);
                    blip.SetColour(BlipColour::Pink);
                    blip.SetFriendly(true);
                }
            }
        }
        return;
    }

    if (pvTeleportIntoIt)
    {
        if (!pv.Exists())
        {
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.noLongerInMemory"));
        }
        else
        {
            myPed.SetIntoVehicle(pv, pv.FirstFreeSeat(SEAT_DRIVER));
        }
        return;
    }

    if (pvTeleportToIt)
    {
        if (!pv.Exists())
        {
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.noLongerInMemory"));
        }
        else
        {
            if (myPed.IsInVehicle())
            {
                myVehicle.SetPosition(pv.GetOffsetInWorldCoords(0, pv.Dim1().y + myVehicle.Dim2().y + 0.3f, 0));
            }
            else
            {
                myPed.SetPosition(pv.GetOffsetInWorldCoords(0, 0, pv.Dim2().z + myPed.Dim1().z));
            }
        }
        return;
    }

    if (pvTeleportItToYou)
    {
        if (!pv.Exists())
        {
            Game::Print::ShowNotification(Language::Translate("vehicle.notifications.error.title"), Language::Translate("vehicle.notifications.noLongerInMemory"));
        }
        else
        {
            const Vector3& myPos = myPed.GetPosition();
            pv.RequestControl(600);
            pv.SetPosition(myPos);
        }
        return;
    }
}

namespace VehicleAutoDrive
{
class MethodsClass final : public GenericLoopedMode
{
  private:
    GTAped myPed;
    GTAvehicle vehicle;
    Model vehicleModel;
    Vector3 destination;

  public:
    float speed = 20;
    int drivingStyle = 5;
    UINT8 drivingStyleIndex = 0;
    bool bPushEmAway = true;
    bool bRandDestinationMode = false;
    float pushRadius = 4.0f;
    bool initialSet = false;

    void TurnOn() override
    {
        GenericLoopedMode::TurnOn();

        initialSet = false;
    }

    void TurnOff() override
    {
        GenericLoopedMode::TurnOff();

        initialSet = false;

        CLEAR_PED_TASKS(myPed.Handle());
    }

    void Tick() override
    {
        if (bEnabled)
        {
            MainTick();
        }
    }

    inline void MainTick()
    {
        myPed = PLAYER_PED_ID();

        if (myPed.IsInVehicle() && (IS_WAYPOINT_ACTIVE() || bRandDestinationMode))
        {
            if (!initialSet)
            {
                destination = GET_BLIP_COORDS(GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint));
            }

            vehicle = myPed.CurrentVehicle();
            vehicleModel = vehicle.Model();
            Vector3 currentPos = vehicle.GetPosition();

            // If we are within 300m of destination OR outside map bounds, pick a new target
            bool outOfBounds = (currentPos.x > 4000.0f || currentPos.x < -4000.0f || currentPos.y > 7000.0f || currentPos.y < -3500.0f);
            bool arrived = currentPos.DistanceTo(destination) < 300.0f;

            if (bRandDestinationMode && (vehicleModel.IsHeli() || vehicleModel.IsPlane()))
            {
                if (arrived || outOfBounds || !initialSet)
                {
                    if (outOfBounds)
                    { // out of bounds, target the center of the map (0,0)
                        destination = {0.0f, 0.0f, 400.0f};
                    }
                    else
                    {
                        int currentDir = MISC::GET_RANDOM_INT_IN_RANGE(1, 5);
                        float offset = 4000.0f;
                        float targetZ = 400.0f; // Sane cruising altitude to avoid mountains

                        if (currentDir == 1)
                            destination = {currentPos.x, currentPos.y + offset, targetZ}; // N
                        else if (currentDir == 2)
                            destination = {currentPos.x + offset, currentPos.y, targetZ}; // E
                        else if (currentDir == 3)
                            destination = {currentPos.x, currentPos.y - offset, targetZ}; // S
                        else
                            destination = {currentPos.x - offset, currentPos.y, targetZ}; // W
                    }
                    initialSet = false;
                }
            }
            else if (!initialSet && IS_WAYPOINT_ACTIVE())
            {
                destination = GET_BLIP_COORDS(GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint));
            }

            if (vehicleModel.IsHeli())
                HeliTick();
            else if (vehicleModel.IsPlane())
                PlaneTick();
            else if (vehicleModel.IsBoat())
                BoatTick();
            else
                NormalTick();

            if (bPushEmAway)
            {
                PushEmAway();
            }
        }
        else
        {
            TurnOff();
        }
    }

    inline void NormalTick()
    {
        if (!initialSet)
        {
            ScrHandle tsk;
            OPEN_SEQUENCE_TASK(&tsk);

            if (bRandDestinationMode)
            {
                TASK_VEHICLE_DRIVE_WANDER(0, vehicle.Handle(), speed, drivingStyle);
            }
            else
            {
                addlog(ige::LogType::LOG_DEBUG, "Driving to: " + std::to_string(destination.x) + ", " + std::to_string(destination.y) + ", " + std::to_string(destination.z));
                TASK_VEHICLE_DRIVE_TO_COORD_LONGRANGE(0, vehicle.Handle(), destination.x, destination.y, destination.z, speed, drivingStyle, 7.0f);
            }
            CLOSE_SEQUENCE_TASK(tsk);
            TASK_PERFORM_SEQUENCE(myPed.Handle(), tsk);
            CLEAR_SEQUENCE_TASK(&tsk);

            initialSet = true;
        }
    }

    inline void HeliTick()
    {
        if (!initialSet)
        {
            ScrHandle tsk;
            OPEN_SEQUENCE_TASK(&tsk);
            addlog(ige::LogType::LOG_DEBUG, "Heli Flying to: " + std::to_string(destination.x) + ", " + std::to_string(destination.y) + ", " + std::to_string(destination.z));

            TASK_HELI_MISSION(0, vehicle.Handle(), 0, 0, destination.x, destination.y, destination.z, 9, speed, 250.0f, -1.0f, 100, 200, 0xbf800000, 0);

            CLOSE_SEQUENCE_TASK(tsk);
            TASK_PERFORM_SEQUENCE(myPed.Handle(), tsk);
            CLEAR_SEQUENCE_TASK(&tsk);

            initialSet = true;
        }
    }

    inline void PlaneTick()
    {
        if (!initialSet)
        {
            ScrHandle tsk;
            OPEN_SEQUENCE_TASK(&tsk);
            addlog(ige::LogType::LOG_DEBUG, "Plane Flying to: " + std::to_string(destination.x) + ", " + std::to_string(destination.y) + ", " + std::to_string(destination.z));

            TASK_PLANE_MISSION(0, vehicle.Handle(), 0, 0, destination.x, destination.y, destination.z, 4, speed, 50.0f, -1.0f, 100.0f, 200.0f, false);

            CLOSE_SEQUENCE_TASK(tsk);
            TASK_PERFORM_SEQUENCE(myPed.Handle(), tsk);
            CLEAR_SEQUENCE_TASK(&tsk);

            initialSet = true;
        }
    }

    inline void BoatTick()
    {
        if (!initialSet)
        {
            ScrHandle tsk;
            OPEN_SEQUENCE_TASK(&tsk);

            if (bRandDestinationMode)
            {
                TASK_VEHICLE_DRIVE_WANDER(0, vehicle.Handle(), speed, drivingStyle);
            }
            else
            {
                addlog(ige::LogType::LOG_DEBUG, "Sailing to: " + std::to_string(destination.x) + ", " + std::to_string(destination.y) + ", " + std::to_string(destination.z));
                TASK_BOAT_MISSION(0, vehicle.Handle(), 0, 0, destination.x, destination.y, destination.z, 4, speed, 786469, 10.0f, 1071);
            }
            CLOSE_SEQUENCE_TASK(tsk);
            TASK_PERFORM_SEQUENCE(myPed.Handle(), tsk);
            CLEAR_SEQUENCE_TASK(&tsk);

            initialSet = true;
        }
    }

    inline void PushEmAway()
    {
        const auto& md = vehicleModel.Dimensions();
        const auto& pos = vehicle.GetPosition();
        const auto& rot = vehicle.GetRotation();
        const auto& dir = Vector3::RotationToDirection(rot);

        auto ray = RaycastResult::RaycastCapsule(pos, dir, 3.2f + md.Dim1.y, 2.3f, IntersectOptions::Everything, vehicle);

        if (ray.DidHitEntity())
        {
            GTAentity thingInFront = ray.HitEntity();
            thingInFront.ApplyForce(dir * 10.0f);
        }

        const Vector3& myFrontBumper = pos + (dir * md.Dim1.y);

        for (GTAvehicle v : nearbyVehicles)
        {
            if (v.Handle() == vehicle.Handle())
            {
                continue;
            }
            if (v.IsInRangeOf(myFrontBumper, pushRadius))
            {
                v.ApplyForce(dir * 10.0f);
            }
        }
    }
};

MethodsClass Methods;

void ToggleOnOff()
{
    Methods.Toggle();
}

void Tick()
{
    Methods.Tick();
}

void AutoDriveMenu()
{
    GTAped myPed = Game::PlayerPed();

    auto& speed = Methods.speed;
    auto& bRandDestinationMode = Methods.bRandDestinationMode;
    auto& drivingStyle = Methods.drivingStyle;
    auto& drivingStyleIndex = Methods.drivingStyleIndex;
    auto& bPushEmAway = Methods.bPushEmAway;
    auto& pushRadius = Methods.pushRadius;

    std::vector<std::string> drivingStyleNames;
    for (auto& dsns : DrivingStyle::nameArray)
    {
        drivingStyleNames.push_back(dsns.name);
    }

    bool speedPlus = false;
    bool speedMinus = false;
    bool drivingStylePlus = false;
    bool drivingStyleMinus = false;
    bool radiusPlus = false;
    bool radiusMinus = false;

    AddTitle(Language::Translate("vehicle.autoDrive.title"));

    AddToggle(Language::Translate("vehicle.autoDrive.randomDestination.label"), bRandDestinationMode, ToggleOnOff, ToggleOnOff);
    AddOptionDescription(Language::Translate("vehicle.autoDrive.randomDestination.description"));
    AddLocal(Language::Translate("vehicle.autoDrive.goToWaypoint.label"), Methods.Enabled() && !bRandDestinationMode, ToggleOnOff, ToggleOnOff);
    AddNumber(Language::Translate("vehicle.autoDrive.speed.label"), speed * 3.6f, 1, null, speedPlus, speedMinus);
    AddTexter(Language::Translate("vehicle.autoDrive.drivingStyle.label"), drivingStyleIndex, drivingStyleNames, null, drivingStylePlus, drivingStyleMinus);
    AddToggle(Language::Translate("vehicle.autoDrive.pushOtherVehiclesAway.label"), bPushEmAway);
    AddOptionDescription(Language::Translate("vehicle.autoDrive.pushOtherVehiclesAway.description"));
    AddNumber(Language::Translate("vehicle.autoDrive.forcefieldRadius.label"), pushRadius, 1, null, radiusPlus, radiusMinus);
    AddOptionDescription(Language::Translate("vehicle.autoDrive.forcefieldRadius.description"));

    if (speedPlus)
    {
        if (speed < FLT_MAX)
        {
            speed += (0.5f / 3.6f);
        }
        myPed.SetDrivingSpeed(speed);
        return;
    }
    if (speedMinus)
    {
        if (speed > 0)
        {
            speed -= (0.5f / 3.6f);
        }
        myPed.SetDrivingSpeed(speed);
        return;
    }

    if (drivingStylePlus)
    {
        if (drivingStyleIndex < drivingStyleNames.size() - 1)
        {
            drivingStyleIndex++;
        }
        myPed.SetDrivingStyle(DrivingStyle::nameArray[drivingStyleIndex].style);
        return;
    }
    if (drivingStyleMinus)
    {
        if (drivingStyleIndex > 0)
        {
            drivingStyleIndex--;
        }
        myPed.SetDrivingStyle(DrivingStyle::nameArray[drivingStyleIndex].style);
        return;
    }

    if (radiusPlus)
    {
        pushRadius += 0.5f;
        return;
    }
    if (radiusMinus)
    {
        if (pushRadius > 1.0f)
            pushRadius -= 0.5f;
        return;
    }
}
} // namespace VehicleAutoDrive

void VehicleMultipliersMenu()
{
    bool accelerationMultiplierPlus = false;
    bool accelerationMultiplierMinus = false;
    bool brakeMultiplierPlus = false;
    bool brakeMultiplierMinus = false;
    bool handlingMultiplierPlus = false;
    bool handlingMultiplierMinus = false;

    AddTitle(Language::Translate("vehicle.multipliers.title"));
    AddNumber("CMOD_STAT_1", accelMult, 0, null, accelerationMultiplierPlus, accelerationMultiplierMinus, true); // Acceleration
    AddOptionDescription(Language::Translate("vehicle.multipliers.acceleration.description"));
    AddNumber(Game::GetGXTEntry("CMOD_STAT_2") + " & Reverse", brakeMult, 0, null, brakeMultiplierPlus, brakeMultiplierMinus); // Braking & Reverse
    AddOptionDescription(Language::Translate("vehicle.multipliers.braking.description"));
    AddNumber("CMOD_STAT_3", handlingMult, 0, null, handlingMultiplierPlus, handlingMultiplierMinus, true); // Handling/Traction
    AddOptionDescription(Language::Translate("vehicle.multipliers.handling.description"));


    if (accelerationMultiplierPlus)
    {
        if (accelMult < 200)
        {
            accelMult++;
            return;
        }
    }

    if (accelerationMultiplierMinus)
    {
        if (accelMult > 0)
            accelMult--;
        return;
    }

    if (brakeMultiplierPlus)
    {
        if (brakeMult < 100)
        {
            brakeMult++;
            return;
        }
    }

    if (brakeMultiplierMinus)
    {
        if (brakeMult > 0)
        {
            brakeMult--;
            return;
        }
    }

    if (handlingMultiplierPlus)
    {
        if (handlingMult < 100)
        {
            handlingMult++;
            return;
        }
    }
    if (handlingMultiplierMinus)
    {
        if (handlingMult > 0)
        {
            handlingMult--;
            return;
        }
    }
}

void VehicleMultiPlatNeonsMenu()
{
    bool addCar = 0, setRGBIndexTo3 = 0, clearList = 0, intensityPlus = 0, intensityMinus = 0, intensityCustom = 0;

    AddTitle(Language::Translate("vehicle.neons.title"));
    AddToggle(Language::Translate("vehicle.neons.toggle.label"), multiPlatNeons, null, clearList);
    AddOptionDescription(Language::Translate("vehicle.neons.toggle.description"));
    AddNumber(Language::Translate("vehicle.neons.intensity.label"), g_multiPlatNeonsIntensity, 2, intensityCustom, intensityPlus, intensityMinus);
    AddOptionDescription(Language::Translate("vehicle.neons.intensity.description"));

    AddOption(Language::Translate("vehicle.neons.setColour.label"), setRGBIndexTo3, nullFunc, SUB::MSPAINTS_RGB);
    if (Menu::IsLastDrawnOptionSelected())
    {
        AddPresetColourOptionsPreview(g_multiPlatNeonsColor);
    }

    if (intensityPlus)
    {
        g_multiPlatNeonsIntensity += 0.05f;
    }
    if (intensityMinus)
    {
        g_multiPlatNeonsIntensity -= 0.05f;
    }

    if (intensityCustom)
    {
        std::string inputStr = Game::InputBox(std::to_string(g_multiPlatNeonsIntensity), 10U, Language::Translate("common.input.valuePrompt").c_str(), std::to_string(g_multiPlatNeonsIntensity));
        if (inputStr.length() > 0)
        {
            try
            {
                g_multiPlatNeonsIntensity = stof(inputStr);
            }
            catch (...)
            {
                Game::Print::PrintErrorInvalidInput(inputStr);
            }
        }
    }

    if (setRGBIndexTo3)
    {
        bitMSPaintsRGBMode = 3;
    }

    if (clearList)
    {
        g_multiPlatNeonsList.clear();
    }
}

namespace Speedo
{

struct NamedSpeedoImage
{
    std::string displayName;
    std::string fileName;
    DxHookIMG::DxTexture id;
};

std::vector<NamedSpeedoImage> speedoImagesNames[]{
    {{"Orange", "needle_orange", 0}, {"Orange Night", "night_needle_orange", 0}, {"Red", "needle_red", 0}, {"Red Night", "night_needle_red", 0}},
    {{"Cyan", "bg_cyan2", 0}, {"Green", "bg_green2", 0}, {"Yellow", "bg_yellow2", 0}, {"Orange", "bg_orange2", 0}, {"Purple", "bg_purple2", 0}, {"Pink", "bg_pink2", 0}},
    {{"Cyan", "bg_cyan", 0}, {"Green", "bg_green", 0}, {"Yellow", "bg_yellow", 0}, {"Orange", "bg_orange", 0}, {"Purple", "bg_purple", 0}, {"Pink", "bg_pink", 0}}
};

SpeedoImage currentSpeedoBG = {speedoImagesNames[2].at(0).fileName, 0};
SpeedoImage currentSpeedoNeedle = {speedoImagesNames[0].at(0).fileName, 0};
UINT8 speedoAlpha = 0;
Vector2 speedoPosition = {0.85f, 0.86f};

UINT8 loopSpeedo = SPEEDOMODE_OFF;
bool speedoMPH = false;

void SetCurrentBgIdFromBgNameForConfig()
{
    for (auto& sic : speedoImagesNames)
    {
        for (auto& si : sic)
        {
            if (currentSpeedoBG.fileName == si.fileName)
            {
                currentSpeedoBG.id = si.id;
                return;
            }
        }
    }
}

void LoadSpeedoImages()
{
    for (auto& sic : speedoImagesNames)
    {
        for (auto& si : sic)
        {
            si.id.Load(GetPathffA(Pathff::Speedo, true) + si.fileName + ".png");
        }
    }

    SetCurrentBgIdFromBgNameForConfig();
}

inline void DrawSpeedoImage(SpeedoImage& bg, SpeedoImage& needle, float speedf, float alpha)
{
    Vector2 size = {0.1540f, 0.164f};
    Vector2& pos = Speedo::speedoPosition;

    bg.id.Draw(0, pos, size, 0.0f, RGBA(255, 255, 255, alpha < 170 ? alpha : 170));
    needle.id.Draw(0, pos, size, (speedf > 270.0f ? 270.6f : speedf), RGBA(255, 255, 255, alpha));

    Game::Print::SetupDraw(font_speedo, Vector2(0.33, 0.33), true, false, false, RGBA(0, 153, 153, alpha));
    Game::Print::drawfloat(speedf, 0, pos.x, pos.y - 0.073f);
}

void SpeedoTick()
{
    if (!IS_PLAYER_CONTROL_ON(PLAYER_ID()))
    {
        speedoAlpha = 0;
    }
    else
    {
        float speedf = abs(3.6f * GET_ENTITY_SPEED_VECTOR(g_myVeh, true).y);
        if (speedoMPH)
        {
            speedf *= 0.6214f;
        }

        if (loopSpeedo == SPEEDOMODE_DIGITAL)
        {
            std::string speedStr;
            if (speedoMPH)
            {
                speedStr = std::to_string((int)speedf) + " ~b~MPH";
            }
            else
            {
                speedStr = std::to_string((int)speedf) + " ~b~KMPH";
            }
            Game::Print::SetupDraw(font_speedo, Vector2(0.8f, 0.8f), true, false, false);
            Game::Print::drawstring(speedStr, 0.915f, 0.8f);
        }
        else
        {
            if (speedoAlpha < 255)
            {
                speedoAlpha += 5;
            }

            UINT8 clockHour = GET_CLOCK_HOURS();
            if (clockHour < 19 && clockHour > 7)
            {
                currentSpeedoNeedle = {speedoImagesNames[0].at(0).fileName, speedoImagesNames[0].at(0).id}; // Day
            }
            else
            {
                currentSpeedoNeedle = {speedoImagesNames[0].at(1).fileName, speedoImagesNames[0].at(1).id}; // Night
            }

            DrawSpeedoImage(currentSpeedoBG, currentSpeedoNeedle, speedf, speedoAlpha);
        }
    }
}

void AddOptionSpeedo(const std::string& text, NamedSpeedoImage& img)
{
    bool pressed = false;
    AddTickol(text, currentSpeedoBG.id.ID() == img.id.ID(), pressed, pressed);
    if (pressed)
    {
        currentSpeedoBG = {img.fileName, img.id};
    }
}

void SpeedoMainMenu()
{
    std::vector<std::string> vSpeedoUnits{Language::Translate("vehicle.speedometers.unit.metric"), Language::Translate("vehicle.speedometers.unit.imperial")};
    std::vector<std::string> vSpeedoTnames{
        Language::Translate("vehicle.speedometers.mode.off"), Language::Translate("vehicle.speedometers.mode.digital"), Language::Translate("vehicle.speedometers.mode.analogue")
    };

    bool speedoModePlus = 0;
    bool speedoModeMinus = 0;
    bool speedoUnitPlus = 0;
    bool asPosXPlus = 0;
    bool asPosXMinus = 0;
    bool asPosYPlus = 0;
    bool asPosYMinus = 0;

    AddTitle(Language::Translate("vehicle.speedometers.title"));
    AddTexter(Language::Translate("vehicle.speedometers.speedo.label"), loopSpeedo, vSpeedoTnames, null, speedoModePlus, speedoModeMinus);
    AddTexter(Language::Translate("vehicle.speedometers.unit.label"), speedoMPH, vSpeedoUnits, null, speedoUnitPlus, speedoUnitPlus);
    if (loopSpeedo == SPEEDOMODE_ANALOGUE)
    {
        AddOption(Language::Translate("vehicle.speedometers.lightThemes.label"), null, nullFunc, SUB::VEHICLESPEEDOS_LIGHT);
        AddOption(Language::Translate("vehicle.speedometers.darkThemes.label"), null, nullFunc, SUB::VEHICLESPEEDOS_DARK);
        AddNumber(Language::Translate("vehicle.speedometers.xPosition.label"), speedoPosition.x, 3, null, asPosXPlus, asPosXMinus);
        AddNumber(Language::Translate("vehicle.speedometers.yPosition.label"), speedoPosition.y, 3, null, asPosYPlus, asPosYMinus);
    }

    if (asPosXPlus)
    {
        speedoPosition.x += 0.005f;
    }
    else if (asPosXMinus)
    {
        speedoPosition.x -= 0.005f;
    }
    if (asPosYPlus)
    {
        speedoPosition.y += 0.005f;
    }
    else if (asPosYMinus)
    {
        speedoPosition.y -= 0.005f;
    }

    if (speedoModePlus)
    {
        if (loopSpeedo < vSpeedoTnames.size() - 1)
        {
            loopSpeedo++;
        }
    }
    if (speedoModeMinus)
    {
        if (loopSpeedo > 0)
        {
            loopSpeedo--;
        }
    }

    if (speedoUnitPlus)
    {
        speedoMPH = speedoMPH == 0 ? 1 : 0;
    }
}

void SpeedoThemesLight()
{
    AddTitle(Language::Translate("vehicle.speedometers.lightThemes.title"));

    for (auto& si : speedoImagesNames[1])
    {
        AddOptionSpeedo(si.displayName, si);
    }
}

void SpeedoThemesDark()
{
    AddTitle(Language::Translate("vehicle.speedometers.darkThemes.title"));

    for (auto& si : speedoImagesNames[2])
    {
        AddOptionSpeedo(si.displayName, si);
    }
}
} // namespace Speedo
} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(VEHICLEOPS, sub::VehicleMenu)
REGISTER_SUBMENU(PVOPS, sub::PVMenu)
REGISTER_SUBMENU(VEHICLEMULTIPLIERS, sub::VehicleMultipliersMenu)
REGISTER_SUBMENU(VEHICLESPEEDOS, sub::Speedo::SpeedoMainMenu)
REGISTER_SUBMENU(VEHICLESPEEDOS_LIGHT, sub::Speedo::SpeedoThemesLight)
REGISTER_SUBMENU(VEHICLESPEEDOS_DARK, sub::Speedo::SpeedoThemesDark)
REGISTER_SUBMENU(MULTIPLATNEONS, sub::VehicleMultiPlatNeonsMenu)
REGISTER_SUBMENU(AUTODRIVESUB, sub::VehicleAutoDrive::AutoDriveMenu)
