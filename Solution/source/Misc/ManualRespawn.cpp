/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "ManualRespawn.h"

#include "..\macros.h"

#include "..\Natives\natives2.h"
#include "..\Scripting\enums.h"
#include "..\Scripting\Game.h"
#include "..\Menu\Menu.h"
#include "..\Menu\Language.h"
#include "..\Menu\Keybinds.h"
#include "..\Util\FileLogger.h"

// death model check
#include "..\Scripting\Model.h"
#include "..\Scripting\GTAplayer.h"
#include "..\Scripting\GTAped.h"
#include "..\Util\ExePath.h"
#include "..\Scripting\CustomHelpText.h"
#include "..\Scripting\WeaponIndivs.h"

#include "..\Submenus\Wardrobe\PedOutfitPersistence.h"

#include <vector>

namespace ManualRespawn
{
ManualRespawn::ManualRespawn() : inRespawn(false)
{
}

bool ManualRespawn::InRespawn()
{
    return inRespawn;
}

void ManualRespawn::TurnOff()
{
    GenericLoopedMode::TurnOff();

    PAUSE_DEATH_ARREST_RESTART(false);
    SET_NO_LOADING_SCREEN(false);
    SET_FADE_OUT_AFTER_DEATH(true);
    SET_FADE_OUT_AFTER_ARREST(true);
}

bool ManualRespawn::IsSkipPressed()
{
    return Keybinds::WasPressedThisFrame("respawn");
}

inline void ManualRespawn::ShowRespawnHelpText()
{
    Game::Print::SetupDraw(GTAfont::Arial, Vector2(0, 0.4f), false, true, false, RGBA(255, 255, 255, 190));
    Game::Print::drawstring("Press ~b~[" + Keybinds::GetGlyph("respawn") + "]~s~ to respawn.", NULL, 0.1f);
}

void ManualRespawn::Tick()
{
    if (bEnabled)
    {
        DoManualRespawnTick();
    }
}

inline void ManualRespawn::DoManualRespawnTick()
{
    auto player = PLAYER_ID();

    if (IS_PLAYER_DEAD(player))
    {
        if (IsSkipPressed())
        {
            PAUSE_DEATH_ARREST_RESTART(false);
            SET_NO_LOADING_SCREEN(false);
            SET_FADE_OUT_AFTER_DEATH(true);
            SET_FADE_OUT_AFTER_ARREST(true);
            DO_SCREEN_FADE_OUT(4000);
            inRespawn = true;
        }
        else if (!inRespawn)
        {
            PAUSE_DEATH_ARREST_RESTART(true);
            SET_NO_LOADING_SCREEN(true);
            SET_FADE_OUT_AFTER_DEATH(false);
            SET_FADE_OUT_AFTER_ARREST(false);
            if (IS_SCREEN_FADING_OUT())
                DO_SCREEN_FADE_IN(100);
            ShowRespawnHelpText();
        }
    }
    else
    {
        inRespawn = false;
    }
}

ManualRespawn g_manualRespawn;

void ToggleOnOff()
{
    g_manualRespawn.Toggle();
}

void CheckSelfDealthModel()
{
    GTAped playerPed = PLAYER_PED_ID();

    if (!playerPed.Exists())
        return;

    GTAplayer player = PLAYER_ID();

    bool is_death = playerPed.IsDead();
    bool is_arrest = IS_PLAYER_BEING_ARRESTED(player.Handle(), true) != 0;

    if ((!is_death && !is_arrest) || g_manualRespawn.InRespawn())
        return;
    if (GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH(0xCAC8014F) > 0) //director_mode.ysc
        return;

    Model model = playerPed.Model();
    Model orig_model;


    if (model.hash != PedHash::Michael && model.hash != PedHash::Franklin && model.hash != PedHash::Trevor)
    {
        std::vector<s_Weapon_Components_Tint> weaponsBackup;
        playerPed.StoreWeaponsInArray(weaponsBackup);

        const std::string& ofn = GetPathffA(Pathff::Outfit, true) + "_reserved.xml";
        const bool outfitSaved = sub::PedOutfitPersistence::SavePedOutfit(GTAped(PLAYER_PED_ID()), ofn);
        if (!outfitSaved)
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.create_file_failed.message"));


        if (is_death)
            WAIT(8500);  // wait until screen turns black
        else             // is_arrest
            WAIT(10000); // wait until screen turns black

        // load a default model
        orig_model = PedHash::Michael;
        orig_model.LoadAndWait();
        SET_PLAYER_MODEL(PLAYER_ID(), orig_model.hash);
        GTAped(PLAYER_PED_ID()).SetDefaultComponentVariation();
        orig_model.Unload();

        // wait until player has resurrected
        if (is_death)
            while (GET_TIME_SINCE_LAST_DEATH() <= 0 || GET_TIME_SINCE_LAST_DEATH() > 6000)
                WAIT(0);
        else
            while (GET_TIME_SINCE_LAST_ARREST() <= 0 || GET_TIME_SINCE_LAST_ARREST() > 6000)
                WAIT(0);

        while (!player.IsControlOn())
            WAIT(0);

        // change back the model
        if (outfitSaved)
        {
            const bool outfitApplied = sub::PedOutfitPersistence::ApplyPedOutfit(GTAped(PLAYER_PED_ID()), ofn, sub::PedOutfitPersistence::OutfitApplyOptions::CompleteWithoutAttachments());
            if (!outfitApplied)
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.load_failed.message"));
        }

        WAIT(0);

        player = PLAYER_ID();
        playerPed = PLAYER_PED_ID();

        playerPed.GiveWeaponsFromArray(weaponsBackup);

        Game::CustomHelpText::ShowTimedText("If you are having issues at this point, load a saved game.", 7000);
    }
}

} // namespace ManualRespawn
