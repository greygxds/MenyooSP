/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "PlayerOptions.h"
#include "..\Menu\Language.h"
#include "Spooner/Submenus.h"

namespace sub
{
void PlayerOptionsMenu()
{
    dict2.clear();

    bool goToAlphaLevel = false;
    bool replenishPlayer = false;
    bool maxAllStats = false;
    bool invincibilityOff = false;
    bool invisibilityOff = false;
    bool noRagdollOff = false;
    bool seatbeltOff = false;
    bool ignoredByEveryoneOff = false;
    bool neverWantedOn = false;
    bool neverWantedOff = false;
    bool wantedPlus = false;
    bool wantedMinus = false;
    bool wantedFreezeOn = false;
    bool wantedFreezeOff = false;
    bool movementSpeedModifierPlus = false;
    bool movementSpeedModifierMinus = false;
    bool sweatPlus = false;
    bool sweatMinus = false;
    bool noiseValuePlus = false;
    bool noiseValueMinus = false;
    bool collisionOn = false;
    bool collisionOff = false;
    bool heightPlus = false;
    bool heightMinus = false;
    bool movementSpeedPlus = false;
    bool movementSpeedMinus = false;
    bool swimSpeedPlus = false;
    bool swimSpeedMinus = false;

    float& height = g_playerVerticalElongationMultiplier;
    float movementSpeed = 0.0f, swimSpeed = 0.0f;

    bool butAmIOnline = NETWORK_IS_IN_SESSION() != 0;

    g_activePedHandle = PLAYER_PED_ID();
    g_activePlayerId = PLAYER_ID();

    GTAped myPed = g_activePedHandle;
    GTAplayer myPlayer = g_activePlayerId;

    int wantedLevel = myPlayer.GetWantedLevel();

    AddTitle(Language::Translate("menu.player.options.title"));
    AddOption(Language::Translate("menu.player.options.option.basic_stats.label"), null, nullFunc, SUB::SPSTATMANAGER);
    AddOption(Language::Translate("menu.player.options.option.model_changer.label"), null, nullFunc, SUB::MODELCHANGER);

    AddOption(Language::Translate("menu.player.options.option.wardrobe.label"), null, nullFunc, SUB::PED_WARDROBE);

    AddOption(Language::Translate("menu.player.options.option.animations.label"), null, nullFunc, SUB::ANIMATIONSUB);
    AddOption(Language::Translate("menu.player.options.option.scenario_actions.label"), null, nullFunc, SUB::AnimationTaskScenarios);
    AddOption(Language::Translate("menu.player.options.option.moods.label"), null, nullFunc, SUB::FACIALMOOD);
    AddOption(Language::Translate("menu.player.options.option.movement_styles.label"), null, nullFunc, SUB::MOVEMENTGROUP);
    AddOption(Language::Translate("menu.player.options.option.speech_player.label"), null, nullFunc, SUB::SPEECHPLAYER);
    AddOption(Language::Translate("menu.player.options.option.voice_changer.label"), null, nullFunc, SUB::VOICECHANGER);
    AddOption(Language::Translate("menu.player.options.option.ped_flags.label"), null, nullFunc, SUB::PEDFLAGMANAGER_NAMEDLIST);
    AddOptionDescription(Language::Translate("menu.player.options.option.ped_flags.description"));
    AddOption(Language::Translate("menu.player.options.option.trigger_fx.label"), null, nullFunc, SUB::PTFXSUB);
    AddOption(Language::Translate("menu.player.options.option.opacity_local.label"), goToAlphaLevel, nullFunc, SUB::ENTITYALPHALEVEL);
    AddOptionDescription(Language::Translate("menu.player.options.option.opacity_local.description"));
    if (goToAlphaLevel)
    {
        Spooner::Submenus::SetPlayerAsEntityAlphaTarget();
    }

    AddOption(Language::Translate("menu.player.options.option.cloning.label"), null, nullFunc, SUB::CLONECOMPANIONSUB);

    AddOption(Language::Translate("menu.player.options.option.replenish.label"), replenishPlayer);
    AddOptionDescription(Language::Translate("menu.player.options.option.replenish.description"));
    AddOption(Language::Translate("menu.player.options.option.max_all_stats.label"), maxAllStats);
    AddOptionDescription(Language::Translate("menu.player.options.option.max_all_stats.description"));
    AddToggle(Language::Translate("menu.player.options.option.refill_health_in_cover.label"), selfRefillHealthInCover);
    AddOptionDescription(Language::Translate("menu.player.options.option.refill_health_in_cover.description"));
    AddToggle(Language::Translate("menu.player.options.option.invincibility.label"), playerInvincibility, null, invincibilityOff);
    AddLocal(Language::Translate("menu.player.options.option.invisibility.label"), !myPed.IsVisible(), invisibilityOff, invisibilityOff);
    AddToggle(Language::Translate("menu.player.options.option.no_ragdoll.label"), playerNoRagdoll, null, noRagdollOff);
    AddOptionDescription(Language::Translate("menu.player.options.option.no_ragdoll.description"));
    AddToggle(Language::Translate("menu.player.options.option.seatbelt.label"), playerSeatbelt, null, seatbeltOff);
    AddOptionDescription(Language::Translate("menu.player.options.option.seatbelt.description"));
    AddToggle(Language::Translate("menu.player.options.option.unlimited_special_ability.label"), playerUnlimitedAbility);
    AddToggle(Language::Translate("menu.player.options.option.auto_clean.label"), playerAutoClean);
    AddOptionDescription(Language::Translate("menu.player.options.option.auto_clean.description"));
    AddToggle(Language::Translate("menu.player.options.option.ignored_by_everyone.label"), ignoredByEveryone, null, ignoredByEveryoneOff);
    AddOptionDescription(Language::Translate("menu.player.options.option.ignored_by_everyone.description"));
    AddNumber(Language::Translate("menu.player.options.option.wanted_level.label"), wantedLevel, 0, null, wantedPlus, wantedMinus);
    if (wantedLevel > 0)
    {
        AddLocal(Language::Translate("menu.player.options.option.freeze_wanted_level.label"), selfFreezeWantedLevel, wantedFreezeOn, wantedFreezeOff);
        AddOptionDescription(Language::Translate("menu.player.options.option.freeze_wanted_level.description"));
    }
    else
    {
        AddToggle(Language::Translate("menu.player.options.option.never_wanted.label"), neverWanted, neverWantedOn, neverWantedOff);
    }
    AddNumber(Language::Translate("menu.player.options.option.height.label"), height, 2, null, heightPlus, heightMinus);
    AddOptionDescription(Language::Translate("menu.player.options.option.height.description"));
    AddNumber(Language::Translate("menu.player.options.option.movement_speed.label"), swimSpeedMult, 2, null, movementSpeedModifierPlus, movementSpeedModifierMinus);
    AddOptionDescription(Language::Translate("menu.player.options.option.movement_speed.description"));
    AddNumber(Language::Translate("menu.player.options.option.sweat_level.label"), selfSweatMult, 2, null, sweatPlus, sweatMinus);
    AddOptionDescription(Language::Translate("menu.player.options.option.sweat_level.description"));
    AddNumber(Language::Translate("menu.player.options.option.noise_level.label"), playerNoiseMult, 2, null, noiseValuePlus, noiseValueMinus);
    AddOptionDescription(Language::Translate("menu.player.options.option.noise_level.description"));
    AddLocal(Language::Translate("menu.player.options.option.collision.label"), myPed.GetIsCollisionEnabled(), collisionOn, collisionOff);
    AddOptionDescription(Language::Translate("menu.player.options.option.collision.description"));


    if (collisionOn || collisionOff)
    {
        myPed.SetIsCollisionEnabled(!myPed.GetIsCollisionEnabled());
    }

    if (replenishPlayer)
    {
        addlog(ige::LogType::LOG_TRACE, "Replenishing Player");
        myPed.SetHealth(myPed.GetMaxHealth());
        myPed.SetArmour(myPlayer.MaxArmour_get());
        myPed.ClearBloodDamage();
        myPed.ResetVisibleDamage();
        return;
    }

    if (maxAllStats)
    {
        const char* spChars[] = {"SP0_", "SP1_", "SP2_"};
        for (auto& ch : spChars)
        {
            auto setInt = [&](const char* name, int val) { STAT_SET_INT(GET_HASH_KEY(std::string(ch) + name), val, true); };
            auto setFloat = [&](const char* name, float val) { STAT_SET_FLOAT(GET_HASH_KEY(std::string(ch) + name), val, true); };

            setFloat("DIST_RUNNING", 20000.0f);
            setInt("TIME_SWIMMING", 100);
            setInt("TIME_DRIVING_BICYCLE", 100);
            setInt("UNARMED_HITS", 2500);
            STAT_INCREMENT(GET_HASH_KEY(std::string(ch) + "TIME_UNDERWATER"), 6000.0f);
            setInt("NUMBER_NEAR_MISS", 6000);
            setInt("TIME_DRIVING_PLANE", 1000);
            setInt("TIME_DRIVING_HELI", 1000);
            setInt("PLANE_LANDINGS", 100);
            setInt("HITS_MISSION", 5000);
            setInt("HITS_PEDS_VEHICLES", 10000);
            setFloat("DIST_WALK_ST", 5000.0f);
            setInt("KILLS_STEALTH", 200);
            setInt("SPECIAL_ABILITY_UNLOCKED", 100);

            setInt("STAMINA", 100);
            setInt("STRENGTH", 100);
            setInt("LUNG_CAPACITY", 100);
            setInt("WHEELIE_ABILITY", 100);
            setInt("FLYING_ABILITY", 100);
            setInt("SHOOTING_ABILITY", 100);
            setInt("STEALTH_ABILITY", 100);

            STAT_SET_BOOL(GET_HASH_KEY(std::string(ch) + "STAMINA_MAXED"), true, true);
            STAT_SET_BOOL(GET_HASH_KEY(std::string(ch) + "STRENGTH_MAXED"), true, true);
            STAT_SET_BOOL(GET_HASH_KEY(std::string(ch) + "LUNG_CAPACITY_MAXED"), true, true);
            STAT_SET_BOOL(GET_HASH_KEY(std::string(ch) + "WHEELIE_ABILITY_MAXED"), true, true);
            STAT_SET_BOOL(GET_HASH_KEY(std::string(ch) + "FLYING_ABILITY_MAXED"), true, true);
            STAT_SET_BOOL(GET_HASH_KEY(std::string(ch) + "SHOOTING_ABILITY_MAXED"), true, true);
            STAT_SET_BOOL(GET_HASH_KEY(std::string(ch) + "STEALTH_ABILITY_MAXED"), true, true);
        }
        if (!STAT_SAVE_PENDING_OR_REQUESTED())
            STAT_SAVE(0, 0, 3, 0);
        DO_AUTO_SAVE();
        Game::Print::PrintBottomLeft(Language::Translate("message.player.all_stats_maxed.message"));
        return;
    }

    if (invincibilityOff)
    {
        addlog(ige::LogType::LOG_TRACE, "Turning Off Invincibility");
        SET_PLAYER_INVINCIBLE(PLAYER_ID(), 0);
        SetPedInvincibleOff(PLAYER_PED_ID());
        return;
    }

    if (invisibilityOff)
    {
        addlog(ige::LogType::LOG_TRACE, "Turning Off Invisibility");
        myPed.SetVisible(!myPed.IsVisible());
    }

    if (noRagdollOff)
    {
        addlog(ige::LogType::LOG_TRACE, "Turning Off No Ragdoll");
        SetPedNoRagdollOff(PLAYER_PED_ID());
        return;
    }
    if (seatbeltOff)
    {
        addlog(ige::LogType::LOG_TRACE, "Turning Off Seatbelt");
        SetPedSeatbeltOff(PLAYER_PED_ID());
        return;
    }

    if (ignoredByEveryoneOff)
    {
        addlog(ige::LogType::LOG_TRACE, "Ignore By Everyone Off");
        Player temp = PLAYER_ID();
        SET_POLICE_IGNORE_PLAYER(temp, neverWanted);
        SET_EVERYONE_IGNORE_PLAYER(temp, 0);
        SET_PLAYER_CAN_BE_HASSLED_BY_GANGS(temp, 1);
        SET_IGNORE_LOW_PRIORITY_SHOCKING_EVENTS(temp, 0);
        return;
    }

    if (neverWantedOn)
    {
        addlog(ige::LogType::LOG_TRACE, "Never Wanted On");
        SET_PLAYER_WANTED_LEVEL(PLAYER_ID(), 0, 0);
        SET_PLAYER_WANTED_LEVEL_NOW(PLAYER_ID(), 0);
        return;
    }
    if (neverWantedOff)
    {
        addlog(ige::LogType::LOG_TRACE, "Never Wanted Off");
        SET_MAX_WANTED_LEVEL(6);
        SET_WANTED_LEVEL_MULTIPLIER(1.0f);
        return;
    }
    if (wantedPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "Increase Wanted Level");
        if (wantedLevel < 6)
        {
            wantedLevel += 1;
            SET_MAX_WANTED_LEVEL(6);
            SET_PLAYER_WANTED_LEVEL(PLAYER_ID(), wantedLevel, 0);
            SET_PLAYER_WANTED_LEVEL_NOW(PLAYER_ID(), 0);
            neverWanted = false;
        }
        if (selfFreezeWantedLevel != 0)
        {
            selfFreezeWantedLevel = wantedLevel;
        }
        return;
    }
    if (wantedMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "Decrease Wanted Level");
        if (wantedLevel > 0)
        {
            if (wantedLevel == 1)
            {
                wantedLevel -= 1;
                SET_PLAYER_WANTED_LEVEL(PLAYER_ID(), wantedLevel, 0);
            }
            else
            {
                wantedLevel -= 1;
                SET_PLAYER_WANTED_LEVEL(PLAYER_ID(), wantedLevel, 0);
            }
            SET_PLAYER_WANTED_LEVEL_NOW(PLAYER_ID(), 0);
            neverWanted = false;
        }
        if (selfFreezeWantedLevel != 0)
        {
            selfFreezeWantedLevel = wantedLevel;
        }
        return;
    }

    if (wantedFreezeOn)
    {
        addlog(ige::LogType::LOG_TRACE, "Freeze Wanted Level On");
        selfFreezeWantedLevel = Game::Player().GetWantedLevel();
        return;
    }
    if (wantedFreezeOff)
    {
        addlog(ige::LogType::LOG_TRACE, "Freeze Wanted Level Off");
        selfFreezeWantedLevel = 0;
        return;
    }

    if (heightPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "Increase Height");
        if (height < 2.5f)
        {
            height += 0.1f;
            GeneralGlobalHax::SetPlayerHeight(height);
        }
    }
    if (heightMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "Decrease Height");
        if (height > -2.5f)
        {
            height -= 0.1f;
            GeneralGlobalHax::SetPlayerHeight(height);
        }
    }
    if (movementSpeedPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "Increase Movement Speed");
        if (movementSpeed < 9.0f)
        {
            movementSpeed += 0.1f;
            GeneralGlobalHax::SetPlayerMovementSpeed(movementSpeed);
        }
    }
    if (movementSpeedMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "Decrease Movement Speed");
        if (movementSpeed > 0.0f)
        {
            movementSpeed -= 0.1f;
            GeneralGlobalHax::SetPlayerMovementSpeed(movementSpeed);
        }
    }
    if (swimSpeedPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "Increase Swim Speed");
        if (swimSpeed < 9.0f)
        {
            swimSpeed += 0.1f;
            GeneralGlobalHax::SetPlayerSwimSpeed(swimSpeed);
        }
    }
    if (swimSpeedMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "Decrease Swim Speed");
        if (swimSpeed > 0.0f)
        {
            swimSpeed -= 0.1f;
            GeneralGlobalHax::SetPlayerSwimSpeed(swimSpeed);
        }
    }

    if (movementSpeedModifierPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "Increase Movement Speed Modifier");
        if (swimSpeedMult < 1.40f)
        {
            swimSpeedMult += 0.1f;
        }
        return;
    }
    if (movementSpeedModifierMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "Decrease Movement Speed Modifier");
        if (swimSpeedMult > 0.0f)
        {
            swimSpeedMult -= 0.1f;
        }
        return;
    }

    if (sweatPlus)
    {
        addlog(ige::LogType::LOG_TRACE, "Increase Sweat");
        if (selfSweatMult < 5.5f)
        {
            selfSweatMult += 0.1f;
        }
        return;
    }
    if (sweatMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "Decrease Sweat");
        if (selfSweatMult > 0.0f)
        {
            selfSweatMult -= 0.1f;
        }
        if (selfSweatMult == 0.0f)
        {
            myPed.SetSweat(selfSweatMult);
            myPed.ClearWetness();
        }
        return;
    }
    if (noiseValuePlus)
    {
        addlog(ige::LogType::LOG_TRACE, "Increase Noise");
        if (playerNoiseMult < 10.0f)
        {
            playerNoiseMult += 0.1f;
        }
        return;
    }
    if (noiseValueMinus)
    {
        addlog(ige::LogType::LOG_TRACE, "Decrease Noise");
        if (playerNoiseMult > 0.0f)
        {
            playerNoiseMult -= 0.1f;
        }
        return;
    }
}

namespace PedConfigFlagManager
{
std::vector<NamedPedFlagS> pedFlags{
    {ePedConfigFlags::WillFlyThruWindscreen, "Can Fall Out Through Windscreen"},
    {ePedConfigFlags::InVehicle, "Is (Nearby) Car (MAY CRASH)"},
    {ePedConfigFlags::IsAimingGun, "Is Aiming Check"},
    {ePedConfigFlags::ForcedAim, "Is Aiming"},
    {ePedConfigFlags::_0x5FED6BFD, "Has Overflowing Diaper (MAY CRASH)"},
    {ePedConfigFlags::IsInjured, "Is Injured"},
    {ePedConfigFlags::HasHurtStarted, "Is Injured In Combat"},
    {ePedConfigFlags::_Shrink, "Is Short Heighted (Small)"},
};

void FlagListMenu()
{
    GTAped ped = g_activePedHandle;

    AddTitle(Language::Translate("menu.player.ped_flags.title"));
    AddOption(Language::Translate("menu.player.ped_flags.option.custom.label"), null, nullFunc, SUB::PEDFLAGMANAGER_CUSTOM);

    for (auto& f : pedFlags)
    {
        bool bFlagPressed = false;
        BOOL flagStatus = GET_PED_CONFIG_FLAG(ped.Handle(), f.id, true);
        AddLocal(f.title, flagStatus, bFlagPressed, bFlagPressed);
        if (bFlagPressed)
        {
            SET_PED_CONFIG_FLAG(ped.Handle(), f.id, !flagStatus);
        }
    }
}

int flagID = 0;

void CustomFlagSetterMenu()
{
    GTAped ped = g_activePedHandle;
    bool idInput = false;
    bool idPlus = false;
    bool idMinus = false;
    bool idToggle = false;
    BOOL flagStatus = GET_PED_CONFIG_FLAG(ped.Handle(), flagID, true);

    AddTitle(Language::Translate("menu.player.ped_flags.custom.title"));
    AddNumber(Language::Translate("menu.player.ped_flags.custom.option.id.label"), flagID, 0, idInput, idPlus, idMinus);
    AddLocal(Language::Translate("menu.player.ped_flags.custom.option.status.label"), flagStatus, idToggle, idToggle);
    AddOptionDescription(Language::Translate("menu.player.ped_flags.custom.option.status.description"));

    if (idPlus)
    {
        if (flagID < INT_MAX)
        {
            flagID++;
        }
    }
    if (idMinus)
    {
        if (flagID > 0)
        {
            flagID--;
        }
    }
    if (idInput)
    {
        std::string inputStr = Game::InputBox("", 10U);
        if (inputStr.length() > 0)
        {
            try
            {
                flagID = stoi(inputStr);
            }
            catch (...)
            {
                Game::Print::PrintErrorInvalidInput(inputStr);
                addlog(ige::LogType::LOG_ERROR, "Invalid flagID entered: " + inputStr);
            }
        }
    }

    if (idToggle)
    {
        SET_PED_CONFIG_FLAG(ped.Handle(), flagID, !flagStatus);
    }
}
} // namespace PedConfigFlagManager

void CloneCompanionMenu()
{
    GTAplayer player = g_activePlayerId;
    GTAped playerPed = g_activePedHandle;

    if (!playerPed.Exists())
    {
        Game::Print::ShowNotification(Language::Translate("message.player.error.title"), Language::Translate("message.player.no_longer_in_memory.message"));
        addlog(ige::LogType::LOG_WARNING, "Cannot start clone menu, playerPed No longer in memory");
        Menu::SetPreviousMenu();
        return;
    }

    AddTitle(Language::Translate("menu.player.cloning.title"));

    bool cloneNeutralPressed = false;
    AddOption(Language::Translate("menu.player.cloning.option.neutral.label"), cloneNeutralPressed);
    AddOptionDescription(Language::Translate("menu.player.cloning.option.neutral.description"));
    if (cloneNeutralPressed)
    {
        GTAped clone = playerPed.Clone(playerPed.GetHeading(), true, true);
        auto cloneNetId = clone.NetID();
        Game::RequestControlOfId(cloneNetId);
        SET_NETWORK_ID_CAN_MIGRATE(cloneNetId, true);
        SET_NETWORK_ID_EXISTS_ON_ALL_MACHINES(cloneNetId, true);
        clone.SetBlockPermanentEvent(true);
        SET_PED_COMBAT_ABILITY(cloneNetId, 100);
        clone.SetMissionEntity(true);
        clone.NoLongerNeeded();

        Game::Print::PrintBottomLeft(Language::Format("message.player.cloned.message", {{"player", player.GetName()}}));
    }

    bool cloneCompanionPressed = false;
    AddOption(Language::Translate("menu.player.cloning.option.companion.label"), cloneCompanionPressed);
    AddOptionDescription(Language::Translate("menu.player.cloning.option.companion.description"));
    if (cloneCompanionPressed)
    {
        GTAped clone = playerPed.Clone(playerPed.GetHeading(), true, true);
        clone.RequestControl(300);
        auto cloneNetId = clone.NetID();
        Game::RequestControlOfId(cloneNetId);
        SET_NETWORK_ID_CAN_MIGRATE(cloneNetId, true);
        SET_NETWORK_ID_EXISTS_ON_ALL_MACHINES(cloneNetId, true);
        SET_PED_COMBAT_ABILITY(cloneNetId, 200);
        Hash weaponToGive = WEAPON_APPISTOL;
        GIVE_DELAYED_WEAPON_TO_PED(clone.Handle(), weaponToGive, 9999, true);
        clone.SetWeapon(weaponToGive);
        clone.SetFiringPattern(FiringPattern::FullAuto);
        clone.SetShootRate(100);
        sub::Spooner::SpoonerEntity cloneEnt;
        cloneEnt.dynamic = true;
        cloneEnt.handle = clone;
        cloneEnt.hashName = player.GetName() + "\'s Clone Companion";
        cloneEnt.isStill = false;
        cloneEnt.type = EntityType::PED;
        sub::Spooner::EntityManagement::AddEntityToDb(cloneEnt);

        PedGroup grp;
        if (playerPed.IsInGroup())
        {
            grp = playerPed.GetCurrentPedGroup();
        }
        else
        {
            grp = PedGroup::CreateNewGroup();
            grp.Add(playerPed, true);
        }
        grp.Add(clone, false);
        grp.SetSeparationRange(100.0f);
        grp.SetFormationSpacing(1.5f);

        Game::Print::PrintBottomLeft(
            Language::Format("message.player.cloned_companion.message", {{"player", player.GetName()}, {"possessive", playerPed.GetGender() == Gender::Female ? "her" : "his"}})
        );
        Game::Print::PrintBottomLeft(Language::Translate("message.player.clone_added_to_spooner.message"));
    }

    bool cloneEnemyPressed = false;
    AddOption(Language::Translate("menu.player.cloning.option.enemy.label"), cloneEnemyPressed);
    AddOptionDescription(Language::Translate("menu.player.cloning.option.enemy.description"));
    if (cloneEnemyPressed)
    {
        GTAped clone = playerPed.Clone(playerPed.GetHeading(), true, true);
        clone.RequestControl(300);
        auto cloneNetId = clone.NetID();
        Game::RequestControlOfId(cloneNetId);
        SET_NETWORK_ID_CAN_MIGRATE(cloneNetId, true);
        SET_NETWORK_ID_EXISTS_ON_ALL_MACHINES(cloneNetId, true);
        clone.SetBlockPermanentEvent(true);
        SET_PED_COMBAT_ABILITY(cloneNetId, 100);
        Hash weaponToGive = WEAPON_APPISTOL;
        GIVE_DELAYED_WEAPON_TO_PED(clone.Handle(), weaponToGive, 9999, true);
        clone.SetWeapon(weaponToGive);
        clone.SetFiringPattern(FiringPattern::FullAuto);
        clone.SetShootRate(100);
        sub::Spooner::SpoonerEntity cloneEnt;
        cloneEnt.dynamic = true;
        cloneEnt.handle = clone;
        cloneEnt.hashName = player.GetName() + "\'s Clone Enemy";
        cloneEnt.isStill = false;
        cloneEnt.type = EntityType::PED;
        sub::Spooner::EntityManagement::AddEntityToDb(cloneEnt);
        TASK_COMBAT_PED(clone.Handle(), playerPed.Handle(), 0, 16);

        TaskSequence squ;
        TASK_COMBAT_PED_TIMED(0, playerPed.Handle(), 0, 10000);
        TASK_SHOOT_AT_ENTITY(0, playerPed.Handle(), 10000, FiringPattern::FullAuto);
        TASK_PUT_PED_DIRECTLY_INTO_MELEE(0, playerPed.Handle(), 0.0f, -1.0f, 0.0f, 0);
        TASK_COMBAT_HATED_TARGETS_AROUND_PED(0, 25.0f, 0);
        squ.Close(true); // Repeat task sequence if it ends.
        squ.MakePedPerform(clone);
        clone.SetAlwaysKeepTask(true); // May ruin the task sequence. Not sure.
        squ.Clear();

        Game::Print::PrintBottomLeft(Language::Format("message.player.cloned_enemy.message", {{"player", player.GetName()}, {"possessive", playerPed.GetGender() == Gender::Female ? "her" : "his"}}));
        Game::Print::PrintBottomLeft(Language::Translate("message.player.clone_added_to_spooner.message"));
    }
}
} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(CLONECOMPANIONSUB, sub::CloneCompanionMenu)
REGISTER_SUBMENU(PLAYEROPS, sub::PlayerOptionsMenu)
REGISTER_SUBMENU(PEDFLAGMANAGER_NAMEDLIST, sub::PedConfigFlagManager::FlagListMenu)
REGISTER_SUBMENU(PEDFLAGMANAGER_CUSTOM, sub::PedConfigFlagManager::CustomFlagSetterMenu)
