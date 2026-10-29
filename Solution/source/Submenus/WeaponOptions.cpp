/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "WeaponOptions.h"
#include "..\Menu\Language.h"

#include "..\macros.h"

#include "..\Menu\Menu.h"
#include "..\Menu\Keybinds.h"
#include "..\Menu\Routine.h"

#include "..\Natives\natives2.h"
#include "..\Scripting\WeaponIndivs.h"
#include "..\Scripting\GTAplayer.h"
#include "..\Scripting\GTAped.h"
#include "..\Scripting\Game.h"
#include "..\Scripting\Model.h"
#include "..\Util\ExePath.h"
#include "..\Util\StringManip.h"
#include "..\Util\keyboard.h"
#include "..\Menu\FolderPreviewBmps.h"
#include "..\Natives\types.h" //RGBA


#include "VehicleSpawner.h"
#include "Settings.h"

#include <Shlwapi.h> //PathIsDirectory
#pragma comment(lib, "Shlwapi.lib")
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>
#include <pugixml\src\pugixml.hpp>
#include <dirent\include\dirent.h>

int g_WeaponOpsPedOverride = 0;
int g_WeaponOpsPlayerOverride = -1;
Ped g_WeaponMenuPedOverride = 0;

namespace sub
{
static INT s_selectedWeaponCategory = 0;
static INT s_selectedWeaponIndex = 0;

void Weaponops()
{
    dict2.clear();
    dict3.clear();

    if (g_WeaponOpsPedOverride != 0)
    {
        g_activePedHandle = g_WeaponOpsPedOverride;
        g_activePlayerId = g_WeaponOpsPlayerOverride;
    }
    else
    {
        g_activePedHandle = PLAYER_PED_ID();
        g_activePlayerId = PLAYER_ID();
    }


    std::stringstream wdmg_ss;
    wdmg_ss << std::fixed << std::setprecision(2) << (weaponDamageIncrease / 0.72f);
    wdmg_ss << "x";

    bool wdmg_jump = 0, wdmg_plus = 0, wdmg_minus = 0, WeaponopsInfiniteAmmoOff_ = 0, WeaponopsInfiniteAmmoOn_ = 0, bSelfDeleteGunOn = 0, ped_weaps_all = 0, ped_weaps_dlc = 0, ped_weaps_garbage = 0,
         ped_weaps_handcuffs = 0, ped_weaps_digiscanner = 0, ped_weaps_briefcase = 0, bGiveMaxAmmoPressed = 0, WeaponopsRemoveWeaps_ = 0;

    AddTitle(Language::Translate("weapon.options.title"));
    AddOption(Language::Translate("weapon.options.individualWeapons.label"), null, nullFunc, SUB::WEAPONOPS_INDIVS_CATEGORIES);
    AddOption(Language::Translate("weapon.options.loadouts.label"), null, nullFunc, SUB::WEAPONOPS_LOADOUTS);
    //AddOption("Set A Permanent Weapon", null, nullFunc, SUB::WEAPONOPS_FORCEAWEAPON_LIST);
    AddOption(Language::Translate("weapon.options.giveAllWeapons.label"), ped_weaps_all);
    //	AddOption("Give Garbage Bag", ped_weaps_garbage);
    //	AddOption("Give Handcuffs", ped_weaps_handcuffs);
    AddOption(Language::Translate("weapon.options.giveDigiscanner.label"), ped_weaps_digiscanner);
    AddOption(Language::Translate("weapon.options.giveBriefcase.label"), ped_weaps_briefcase);
    AddOption(Language::Translate("weapon.options.giveMaxAmmo.label"), bGiveMaxAmmoPressed);
    AddOption(Language::Translate("weapon.options.removeAllWeapons.label"), WeaponopsRemoveWeaps_);
    AddToggle(Language::Translate("weapon.options.infiniteParachutes.label"), selfInfiniteParachutes);
    AddOptionDescription(Language::Translate("weapon.options.infiniteParachutes.description"));
    AddToggle(Language::Translate("weapon.options.infiniteAmmoInClip.label"), bitInfiniteAmmo, WeaponopsInfiniteAmmoOn_, WeaponopsInfiniteAmmoOff_);
    AddOptionDescription(Language::Translate("weapon.options.infiniteAmmoInClip.description"));
    AddTexter(Language::Translate("weapon.options.weaponDamage.label"), 0, std::vector<std::string>{wdmg_ss.str()}, wdmg_jump, wdmg_plus, wdmg_minus);
    AddOptionDescription(Language::Translate("weapon.options.weaponDamage.description"));
    AddToggle(Language::Translate("weapon.options.entityRemovalGun.label"), selfDeleteGun, bSelfDeleteGunOn);
    AddOptionDescription(Language::Translate("weapon.options.entityRemovalGun.description"));
    if (bSelfDeleteGunOn)
    {
        Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.entityRemovalGunHint"));
        return;
    }

    if (ped_weaps_all)
    {
        GiveAllWeaponsToPed(g_activePedHandle);
        WAIT(15);
        GivePedMaxAmmo(g_activePedHandle);
        return;
    }
    if (bGiveMaxAmmoPressed)
    {
        GivePedMaxAmmo(g_activePedHandle);
        return;
    }
    if (WeaponopsRemoveWeaps_)
    {
        REMOVE_ALL_PED_WEAPONS(g_activePedHandle, 1);
        return;
    }


    if (ped_weaps_garbage)
    {
        GIVE_WEAPON_TO_PED(g_activePedHandle, WEAPON_GARBAGEBAG, 1, true, true);
        SET_CURRENT_PED_WEAPON(g_activePedHandle, WEAPON_GARBAGEBAG, true);
        SET_PED_CURRENT_WEAPON_VISIBLE(g_activePedHandle, 1, 1, 1, 0);
        return;
    }
    if (ped_weaps_handcuffs)
    {
        GIVE_WEAPON_TO_PED(g_activePedHandle, WEAPON_HANDCUFFS, 1, true, true);
        SET_CURRENT_PED_WEAPON(g_activePedHandle, WEAPON_HANDCUFFS, true);
        SET_PED_CURRENT_WEAPON_VISIBLE(g_activePedHandle, 1, 1, 1, 0);
        return;
    }
    if (ped_weaps_digiscanner)
    {
        GIVE_WEAPON_TO_PED(g_activePedHandle, WEAPON_DIGISCANNER, 1, true, true);
        SET_CURRENT_PED_WEAPON(g_activePedHandle, WEAPON_DIGISCANNER, true);
        SET_PED_CURRENT_WEAPON_VISIBLE(g_activePedHandle, 1, 1, 1, 0);
        return;
    }
    if (ped_weaps_briefcase)
    {
        GIVE_WEAPON_TO_PED(g_activePedHandle, WEAPON_BRIEFCASE, 1, true, true);
        SET_CURRENT_PED_WEAPON(g_activePedHandle, WEAPON_BRIEFCASE, true);
        SET_PED_CURRENT_WEAPON_VISIBLE(g_activePedHandle, 1, 1, 1, 0);
        return;
    }

    if (wdmg_jump)
    {
        weaponDamageIncrease = (weaponDamageIncrease == 1.0f ? 100.0f : 1.0f) * 0.72f;
        SET_PLAYER_WEAPON_DAMAGE_MODIFIER(g_activePlayerId, weaponDamageIncrease);
        SET_PLAYER_MELEE_WEAPON_DAMAGE_MODIFIER(g_activePlayerId, weaponDamageIncrease, true);
    }
    if (wdmg_plus)
    {
        if (weaponDamageIncrease / 0.72f < 100.0f)
            weaponDamageIncrease += (0.1f * 0.72f);
        SET_PLAYER_WEAPON_DAMAGE_MODIFIER(g_activePlayerId, weaponDamageIncrease);
        SET_PLAYER_MELEE_WEAPON_DAMAGE_MODIFIER(g_activePlayerId, weaponDamageIncrease, true);
    }
    if (wdmg_minus)
    {
        if (weaponDamageIncrease / 0.72f > -100.0f)
            weaponDamageIncrease -= (0.1f * 0.72f);
        SET_PLAYER_WEAPON_DAMAGE_MODIFIER(g_activePlayerId, weaponDamageIncrease);
        SET_PLAYER_MELEE_WEAPON_DAMAGE_MODIFIER(g_activePlayerId, weaponDamageIncrease, true);
    }

    if (WeaponopsInfiniteAmmoOn_)
    {
        GivePedMaxAmmo(g_activePedHandle);
        SET_PED_INFINITE_AMMO_CLIP(g_activePedHandle, true);
        return;
    }
    if (WeaponopsInfiniteAmmoOff_)
    {
        SET_PED_INFINITE_AMMO_CLIP(g_activePedHandle, false);
        return;
    }
    /*if (WeaponopsInfiniteAmmo_){
		if (!bit_infinite_ammo)
		{
		SET_PED_INFINITE_AMMO_CLIP(Static_241, 1);
		bit_infinite_ammo = true;
		}
		else
		{
		SET_PED_INFINITE_AMMO_CLIP(Static_241, 0);
		bit_infinite_ammo = false;
		}
		return;
		}*/


    //if (laserSightRendering_off) ENABLE_LASER_SIGHT_RENDERING(0);
}

void List_VehicleCats_Sub()
{
    using VehicleSpawner::AddVehicleCategoryOption;
    typedef VehicleSpawner::Indices Indices;

    AddTitle(Language::Translate("weapon.favourites.vehicles.title"));
    AddOption(Language::Translate("weapon.favourites.label"), null, nullFunc, SUB::SPAWNVEHICLE_FAVOURITES);

    AddBreak(Language::Translate("weapon.favourites.vehicles.cars"));
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.openWheel"), Indices::OPENWHEEL);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.super"), Indices::SUPER);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.sports"), Indices::SPORT);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.sportsClassics"), Indices::SPORTSCLASSIC);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.coupes"), Indices::COUPE);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.muscle"), Indices::MUSCLE);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.offroad"), Indices::OFFROAD);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.suvs"), Indices::SUV);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.sedans"), Indices::SEDAN);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.compacts"), Indices::COMPACT);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.drift"), Indices::DRIFT);

    AddBreak(Language::Translate("weapon.favourites.vehicles.industrial"));
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.vans"), Indices::VAN);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.services"), Indices::SERVICE);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.industrial"), Indices::INDUSTRIAL);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.commercial"), Indices::COMMERCIAL);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.utility"), Indices::UTILITY);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.trains"), Indices::TRAIN);

    AddBreak(Language::Translate("weapon.favourites.vehicles.others"));
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.emergency"), Indices::EMERGENCY);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.military"), Indices::MILITARY);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.motorcycles"), Indices::MOTORCYCLE);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.bicycles"), Indices::BICYCLE);
    AddVehicleCategoryOption(Language::Translate("vehicleSpawner.others"), Indices::OTHER);

    /*PCHAR chartick;
		for (const auto& vimg : vVehicleBmps)
		{
		chartick = PCHAR(vimg.dict.c_str());
		if (HAS_STREAMED_TEXTURE_DICT_LOADED(chartick))
		SET_STREAMED_TEXTURE_DICT_AS_NO_LONGER_NEEDED(chartick);
		}*/
}

namespace WeaponFavourites_catind
{
std::string xmlFavouriteWeapons = "FavouriteWeapons.xml";

bool PopulateFavouritesInfo()
{
    pugi::xml_document doc;
    if (doc.load_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str()).status != pugi::status_ok)
        return false;
    pugi::xml_node nodeRoot = doc.document_element();
    for (auto nodeWeap = nodeRoot.child("Weapon"); nodeWeap; nodeWeap = nodeWeap.next_sibling("Weapon"))
    {
        Hash whash = nodeWeap.attribute("hash").as_uint();
        std::string customName = nodeWeap.attribute("customName").as_string();
        bool bIsPresentAlready = WeaponIndivs::vWeaponLabels.count(whash) != 0;
        WeaponIndivs::vWeaponLabels[whash] = customName;
        if (!bIsPresentAlready)
        {
            WeaponAndComponents wac(whash, {}, &WeaponIndivs::vCaptions_Tints);
            WeaponIndivs::vAllWeapons.back()->push_back(wac);
        }
    }
    return true;
}

bool IsWeaponAFavourite(Hash whash)
{
    pugi::xml_document doc;
    if (doc.load_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str()).status != pugi::status_ok)
        return false;
    pugi::xml_node nodeRoot = doc.document_element();
    return nodeRoot.find_child_by_attribute("hash", IntToHexString(whash, true).c_str()) != NULL;
}

bool AddWeaponToFavourites(Hash whash, const std::string& customName)
{
    if (customName.empty())
        return false;

    pugi::xml_document doc;
    if (doc.load_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str()).status != pugi::status_ok)
    {
        doc.reset();
        auto nodeDecleration = doc.append_child(pugi::node_declaration);
        nodeDecleration.append_attribute("version") = "1.0";
        nodeDecleration.append_attribute("encoding") = "ISO-8859-1";
        auto nodeRoot = doc.append_child("FavouriteWeapons");
        doc.save_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str());
    }
    pugi::xml_node nodeRoot = doc.document_element();

    auto nodeOldLoc = nodeRoot.find_child_by_attribute("hash", IntToHexString(whash, true).c_str());
    if (nodeOldLoc) // If not null
    {
        nodeOldLoc.parent().remove_child(nodeOldLoc);
        for (auto wit = WeaponIndivs::vAllWeapons.back()->begin(); wit != WeaponIndivs::vAllWeapons.back()->end(); ++wit)
        {
            if (wit->weaponHash == whash)
            {
                WeaponIndivs::vAllWeapons.back()->erase(wit);
                break;
            }
        }
    }
    auto nodeNewLoc = nodeRoot.append_child("Weapon");
    nodeNewLoc.append_attribute("hash") = IntToHexString(whash, true).c_str();
    nodeNewLoc.append_attribute("customName") = customName.c_str();

    WeaponIndivs::vWeaponLabels[whash] = customName;
    WeaponAndComponents wac(whash, {}, &WeaponIndivs::vCaptions_Tints);
    WeaponIndivs::vAllWeapons.back()->push_back(wac);

    return (doc.save_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str()));
}

bool RemoveWeaponFromFavourites(Hash whash)
{
    pugi::xml_document doc;
    if (doc.load_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str()).status != pugi::status_ok)
        return false;
    pugi::xml_node nodeRoot = doc.document_element();
    auto nodeOldLoc = nodeRoot.find_child_by_attribute("hash", IntToHexString(whash, true).c_str());
    if (nodeOldLoc) // If not null
    {
        nodeOldLoc.parent().remove_child(nodeOldLoc);
    }

    //auto& wlit = WeaponIndivs::vWeaponLabels.find(whash); if (wlit != WeaponIndivs::vWeaponLabels.end())WeaponIndivs::vWeaponLabels.erase(wlit);
    for (auto wit = WeaponIndivs::vAllWeapons.back()->begin(); wit != WeaponIndivs::vAllWeapons.back()->end(); ++wit)
    {
        if (wit->weaponHash == whash)
        {
            WeaponIndivs::vAllWeapons.back()->erase(wit);
            break;
        }
    }

    return (doc.save_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str()));
}

void Sub_WeaponFavourites()
{
    GTAped ped = g_activePedHandle;
    Hash currentPedWeapon = ped.GetWeapon();

    AddTitle(Language::Translate("weapon.favourites.title"));

    pugi::xml_document doc;
    if (doc.load_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str()).status != pugi::status_ok)
    {
        doc.reset();
        auto nodeDecleration = doc.append_child(pugi::node_declaration);
        nodeDecleration.append_attribute("version") = "1.0";
        nodeDecleration.append_attribute("encoding") = "ISO-8859-1";
        auto nodeRoot = doc.append_child("FavouriteWeapons");
        doc.save_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str());
        return;
    }
    pugi::xml_node nodeRoot = doc.document_element();

    bool bInputAdd = false;
    AddOption(Language::Translate("weapon.favourites.addNewWeapon.label"), bInputAdd);
    if (bInputAdd)
    {
        std::string hashNameStr = Game::InputBox("", 40U, Language::Translate("weapon.favourites.weaponName.prompt"));
        if (hashNameStr.length() > 0)
        {
            WAIT(500);
            Hash hashNameHash = GET_HASH_KEY(hashNameStr);
            if (IS_WEAPON_VALID(hashNameHash))
            {
                std::string customNameStr = Game::InputBox("", 28U, Language::Translate("weapon.favourites.customName.prompt"), GetWeaponLabel(hashNameHash, true));
                if (customNameStr.length() > 0)
                {
                    if (AddWeaponToFavourites(hashNameHash, customNameStr))
                    {
                        Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.weaponAdded"));
                    }
                    else
                        Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.unableToAddWeapon"));
                }
                else
                    Game::Print::PrintErrorInvalidInput(customNameStr);
            }
            else
                Game::Print::PrintErrorInvalidInput(std::to_string(hashNameHash));
        }
        else
            Game::Print::PrintErrorInvalidInput(hashNameStr);
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::FavouriteWeaponEntryName, std::string(), 40U, "Enter name (e.g. WEAPON_FLAMETHROWER):");
    }

    if (currentPedWeapon != WEAPON_UNARMED)
    {
        bool bIsCurrentWeaponAFav = IsWeaponAFavourite(currentPedWeapon);
        bool bAddCurrentWeaponToFav = false, bRemoveCurrentWeaponFromFav = false;
        AddTickol(Language::Translate("weapon.favourites.currentWeapon.label"), bIsCurrentWeaponAFav, bAddCurrentWeaponToFav, bRemoveCurrentWeaponFromFav, TICKOL::BOXTICK, TICKOL::BOXBLANK);
        if (bAddCurrentWeaponToFav)
        {
            std::string customNameStr = Game::InputBox("", 28U, Language::Translate("weapon.favourites.customName.prompt"), GetWeaponLabel(currentPedWeapon, true));
            if (customNameStr.length())
            {
                if (AddWeaponToFavourites(currentPedWeapon, customNameStr))
                {
                    Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.weaponAdded"));
                }
                else
                    Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.unableToAddWeapon"));
            }
            else
                Game::Print::PrintErrorInvalidInput(customNameStr);
            //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::FavouriteWeaponCurrent, std::string(), 28U, "Enter custom name:");
            //OnscreenKeyboard::State::arg1._uint = currentPedWeapon;
        }
        else if (bRemoveCurrentWeaponFromFav)
        {
            if (RemoveWeaponFromFavourites(currentPedWeapon))
            {
                Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.weaponRemoved"));
            }
            else
                Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.unableToRemoveWeapon"));
        }
    }

    if (nodeRoot.first_child())
    {
        AddBreak(Language::Translate("weapon.favourites.addedWeapons"));

        for (auto nodeLocToLoad = nodeRoot.first_child(); nodeLocToLoad; nodeLocToLoad = nodeLocToLoad.next_sibling())
        {
            const std::string& customName = nodeLocToLoad.attribute("customName").as_string();
            Model whash = nodeLocToLoad.attribute("hash").as_uint();

            bool bWeapPressed = false;
            AddOption(customName, bWeapPressed);
            if (bWeapPressed)
            {
                INT& selectedCategoryForInItem = s_selectedWeaponCategory;
                INT& selectedWeaponForInItem = s_selectedWeaponIndex;
                auto& vAllAddedWeaponsArr = *WeaponIndivs::vAllWeapons.back();
                for (UINT i = 0; i < vAllAddedWeaponsArr.size(); i++)
                {
                    if (vAllAddedWeaponsArr[i].weaponHash == whash)
                    {
                        selectedCategoryForInItem = (INT)(WeaponIndivs::vAllWeapons.size() - 1);
                        selectedWeaponForInItem = i;
                        Menu::pendingSubmenu = SUB::WEAPONOPS_INDIVS_ITEM;
                        break;
                    }
                }
            }

            if (Menu::IsLastDrawnOptionSelected())
            {
                Keybinds::AddBindIB("menu_action", Language::Translate("menu.remove"));

                if (Keybinds::WasPressedThisFrame("menu_action"))
                {
                    nodeLocToLoad.parent().remove_child(nodeLocToLoad);
                    doc.save_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteWeapons).c_str());
                    if (Menu::IsSelectionAtBottom())
                        Menu::Up();
                    return; // Yeah
                }
            }
        }
    }
}
} // namespace WeaponFavourites_catind

namespace WeaponIndivs_catind
{
//using namespace WeaponIndivs;
using WeaponIndivs::get_weapon;
using WeaponIndivs::vAllWeapons;
using WeaponIndivs::vCaptions_ChuteTints;
using WeaponIndivs::vCategoryNames;
using WeaponIndivs::WEAPONTYPE;

INT& selectedCategory = s_selectedWeaponCategory;
INT& selectedWeapon = s_selectedWeaponIndex;

// THESE SUBMENUS NEED THE AMMU NATION BANNER
void Sub_CategoriesList()
{
    int i;

    if (g_WeaponOpsPedOverride != 0)
    {
        g_activePedHandle = g_WeaponOpsPedOverride;
        g_activePlayerId = g_WeaponOpsPlayerOverride;
    }
    else
    {
        g_activePedHandle = PLAYER_PED_ID();
        g_activePlayerId = PLAYER_ID();
    }

    auto& ped = g_activePedHandle;
    Hash pedCurrentWeapon;
    GET_CURRENT_PED_WEAPON(ped, &pedCurrentWeapon, 1);

    AddTitle(Language::Translate("weapon.individual.title"));

    bool pressedCurrent = false;
    AddOption(Language::Translate("weapon.individual.currentWeapon.label"), pressedCurrent);
    if (pressedCurrent)
    {
        switch (pedCurrentWeapon)
        {
        case WEAPON_UNARMED:
            Game::Print::ShowNotification(Language::Format("weapon.notifications.invalidWeapon", {{"weapon", GetWeaponLabel(pedCurrentWeapon, true)}}));
            break;
        default:
            selectedCategory = WEAPONTYPE::WEAPE_CURRENTLYHELD;
            Menu::pendingSubmenu = (SUB::WEAPONOPS_INDIVS_ITEM);
            break;
        }
    }

    if (GET_PLAYER_PED(g_activePlayerId) == ped)
        AddOption(Language::Translate("weapon.individual.parachutes.label"), null, nullFunc, SUB::WEAPONOPS_PARACHUTE);

    bool pressed = false;
    for (i = 0; i < vCategoryNames.size() - 1 /*Minus Added weapons*/; i++)
    {
        pressed = 0;
        AddOption(vCategoryNames[i], pressed, nullFunc, -1, true);
        if (pressed)
        {
            selectedCategory = i;
            Menu::pendingSubmenu = (SUB::WEAPONOPS_INDIVS_CATEGORY);
        }
    }

    bool bFavsPressed = false;
    AddOption(Language::Translate("weapon.individual.favourites.label"), bFavsPressed, nullFunc, SUB::WEAPONOPS_WEAPONFAVOURITES);
    if (bFavsPressed)
    {
        selectedCategory = (INT)(vCategoryNames.size() - 1);
    }
}

void Sub_InCategory()
{
    int i;

    bool bWeapPressed = false;
    AddTitle(vCategoryNames[selectedCategory]);
    for (i = 0; i < vAllWeapons[selectedCategory]->size(); i++)
    {
        auto& thisWeaponInfo = vAllWeapons[selectedCategory]->at(i);
        bool bWeapPressed = false;
        AddOption(GetWeaponLabel(thisWeaponInfo.weaponHash, true), bWeapPressed);
        if (bWeapPressed)
        {
            selectedWeapon = i;
            Menu::pendingSubmenu = SUB::WEAPONOPS_INDIVS_ITEM;
        }

        if (Menu::IsLastDrawnOptionSelected())
        {
            bool bIsAFav = WeaponFavourites_catind::IsWeaponAFavourite(thisWeaponInfo.weaponHash);
            Keybinds::AddBindIB("menu_action", bIsAFav, Language::Translate("menu.removeFromFavourites"), Language::Translate("menu.addToFavourites"));

            if (Keybinds::WasPressedThisFrame("menu_action"))
            {
                !bIsAFav ? WeaponFavourites_catind::AddWeaponToFavourites(
                               thisWeaponInfo.weaponHash, Game::InputBox("", 28U, Language::Translate("weapon.favourites.customName.prompt"), GetWeaponLabel(thisWeaponInfo.weaponHash, true))
                           )
                         : WeaponFavourites_catind::RemoveWeaponFromFavourites(thisWeaponInfo.weaponHash);
                /*if (!bIsAFav)
						{
						OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::FavouriteWeaponSelected, std::string(), 28U, "Enter custom name:", get_weapon_label(thisWeaponInfo.weaponHash, true));
						OnscreenKeyboard::State::arg1._uint = thisWeaponInfo.weaponHash;
						}
						else WeaponFavourites_catind::RemoveWeaponFromFavourites(thisWeaponInfo.weaponHash);*/
            }
        }
    }
}

void Sub_InItem()
{
    auto& ped = g_activePedHandle;
    NETWORK_REQUEST_CONTROL_OF_ENTITY(ped);

    Hash pedCurrentWeapon;
    GET_CURRENT_PED_WEAPON(ped, &pedCurrentWeapon, 1);

    Hash whash;

    const WeaponAndComponents* selWeapon = nullptr;
    const std::vector<NamedWeaponComponent>* selWeaponComponents = nullptr;
    const std::vector<std::string>* selWeaponTints = nullptr;

    if (selectedCategory == WEAPONTYPE::WEAPE_CURRENTLYHELD)
    {
        whash = pedCurrentWeapon;
        selWeapon = get_weapon(whash);
        if (selWeapon == nullptr)
        {
            Menu::SetPreviousMenu();
            return;
        }
        selWeaponComponents = &selWeapon->components;
        selWeaponTints = selWeapon->tintCaptions;
    }
    else
    {
        selWeapon = &vAllWeapons[selectedCategory]->at(selectedWeapon);
        whash = selWeapon->weaponHash;
        selWeaponComponents = &selWeapon->components;
        selWeaponTints = selWeapon->tintCaptions;
    }

    std::string displayName = GetWeaponLabel(whash, true);

    AddTitle(displayName);

    bool it_Equip = 0;
    AddTickol(Language::Translate("weapon.individual.equipWeapon.label"), pedCurrentWeapon == whash, it_Equip, null);
    if (it_Equip)
    {
        if (DOES_ENTITY_EXIST(ped))
        {
            if (!HAS_PED_GOT_WEAPON(ped, whash, false))
            {
                GIVE_DELAYED_WEAPON_TO_PED(ped, whash, 1000, 0);
                int maxAmmo = 0;
                GET_MAX_AMMO(ped, whash, &maxAmmo);
                SET_AMMO_IN_CLIP(ped, whash, GET_MAX_AMMO_IN_CLIP(ped, whash, false));
                SET_PED_AMMO(ped, whash, maxAmmo, 0);
            }
            SET_CURRENT_PED_WEAPON(ped, whash, true);
        }
    }
    bool it_FillAmmo = 0;
    AddOption(Language::Translate("weapon.individual.fillAmmo.label"), it_FillAmmo);
    AddOptionDescription(Language::Translate("weapon.individual.fillAmmo.description"));
    if (it_FillAmmo)
    {
        int maxAmmo = 0;
        GET_MAX_AMMO(ped, whash, &maxAmmo);
        SET_AMMO_IN_CLIP(ped, whash, GET_MAX_AMMO_IN_CLIP(ped, whash, false));
        SET_PED_AMMO(ped, whash, maxAmmo, 0);
    }

    bool it_FillinfAmmo = 0;
    AddOption(Language::Translate("weapon.individual.emptyAmmo.label"), it_FillinfAmmo);
    AddOptionDescription(Language::Translate("weapon.individual.emptyAmmo.description"));
    if (it_FillinfAmmo)
    {
        SET_AMMO_IN_CLIP(ped, whash, -1);
        SET_PED_AMMO(ped, whash, -1, 0);
    }

    bool it_RemoveWeapon = 0;
    AddOption(Language::Translate("weapon.individual.removeWeapon.label"), it_RemoveWeapon);
    if (it_RemoveWeapon)
    {
        if (HAS_PED_GOT_WEAPON(ped, whash, false))
            REMOVE_WEAPON_FROM_PED(ped, whash);
    }

    if (selWeapon != nullptr)
    {
        if (selWeaponComponents->size() > 0 || selWeaponTints != nullptr)
        {
            AddOption(Language::Translate("weapon.individual.attachmentsTints.label"), null, nullFunc, SUB::WEAPONOPS_INDIVS_ITEM_MODS);
        }
    }
}

void Sub_InItem_Mods()
{
    auto& ped = g_activePedHandle;
    NETWORK_REQUEST_CONTROL_OF_ENTITY(ped);

    Hash pedCurrentWeapon;
    GET_CURRENT_PED_WEAPON(ped, &pedCurrentWeapon, 1);

    Hash whash;

    const WeaponAndComponents* selWeapon = nullptr;
    const std::vector<NamedWeaponComponent>* selWeaponComponents = nullptr;
    const std::vector<std::string>* selWeaponTints = nullptr;

    if (selectedCategory == WEAPONTYPE::WEAPE_CURRENTLYHELD)
    {
        whash = pedCurrentWeapon;
        selWeapon = get_weapon(whash);
        if (selWeapon == nullptr)
        {
            Menu::SetPreviousMenu();
            return;
        }
        selWeaponComponents = &selWeapon->components;
        selWeaponTints = selWeapon->tintCaptions;
    }
    else
    {
        selWeapon = &vAllWeapons[selectedCategory]->at(selectedWeapon);
        whash = selWeapon->weaponHash;
        selWeaponComponents = &selWeapon->components;
        selWeaponTints = selWeapon->tintCaptions;
    }

    auto currentTint = GET_PED_WEAPON_TINT_INDEX(ped, whash);

    AddTitle(Language::Translate("weapon.attachments.title"));

    WEAPON::GIVE_WEAPON_TO_PED(ped, whash, 9999, false, true);
    WEAPON::SET_CURRENT_PED_WEAPON(ped, whash, true);

    for (auto& comp : *selWeaponComponents)
    {
        bool bAttachmentPressed = false;

        if (!WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(whash, comp.hash))
            continue;

        bool bHasComponent = HAS_PED_GOT_WEAPON_COMPONENT(ped, whash, comp.hash) != 0;

        if (bHasComponent && boost::to_upper_copy(comp.name).find("CAMO") != std::string::npos)
        {
            bool bLiveryPressed = false, bLivery_plus = false, bLivery_minus = false;
            int currentLivery = GET_PED_WEAPON_COMPONENT_TINT_INDEX(ped, whash, comp.hash);

            AddTexter(comp.name, 0, std::vector<std::string>{Game::GetGXTEntry("WCT_C_TINT_" + std::to_string(currentLivery))}, bLiveryPressed, bLivery_plus, bLivery_minus);

            if (bLivery_plus && currentLivery < 31)
            {
                currentLivery++;
                bLiveryPressed = true;
            }
            else if (bLivery_minus && currentLivery > 0)
            {
                currentLivery--;
                bLiveryPressed = true;
            }

            if (bLiveryPressed)
            {
                SET_PED_WEAPON_COMPONENT_TINT_INDEX(ped, whash, comp.hash, currentLivery);
            }
        }
        else
        {
            AddTickol(comp.name, bHasComponent, bAttachmentPressed, bAttachmentPressed, TICKOL::WEAPONTHING);

            if (bAttachmentPressed)
            {
                if (bHasComponent)
                    REMOVE_WEAPON_COMPONENT_FROM_PED(ped, whash, comp.hash);
                else
                    GIVE_WEAPON_COMPONENT_TO_PED(ped, whash, comp.hash);
            }
        }
    }

    if (selWeaponTints != nullptr)
    {
        if (!selWeaponComponents->empty())
            AddBreak(Language::Translate("weapon.attachments.tints"));

        for (int i = 0; i < selWeaponTints->size(); i++)
        {
            bool bTintPressed = false;
            AddTickol(selWeaponTints->at(i), (currentTint == i), bTintPressed, null, TICKOL::WEAPONTHING);

            if (bTintPressed)
            {
                SET_PED_WEAPON_TINT_INDEX(ped, whash, i);
            }
        }
    }
}

void Sub_Parachute()
{
    auto& ped = g_activePedHandle;
    NETWORK_REQUEST_CONTROL_OF_ENTITY(ped);
    auto& player = g_activePlayerId;
    UINT i;
    int currentChuteTint;
    GET_PLAYER_PARACHUTE_TINT_INDEX(player, &currentChuteTint);

    RGBA paraSmokeCol;
    GET_PLAYER_PARACHUTE_SMOKE_TRAIL_COLOR(player, &paraSmokeCol.R, &paraSmokeCol.G, &paraSmokeCol.B);

    Hash whash = GADGET_PARACHUTE;
    bool doesPedHavePara = HAS_PED_GOT_WEAPON(ped, whash, false) != 0;

    AddTitle(Language::Translate("weapon.parachute.title"));

    bool it_Equip = 0;
    AddTickol(Language::Translate("weapon.parachute.equipPrimary.label"), doesPedHavePara, it_Equip, it_Equip);
    if (it_Equip)
    {
        if (DOES_ENTITY_EXIST(ped))
        {
            GivePedParachute(ped);
            //SET_PLAYER_PARACHUTE_TINT_INDEX(Static_240, numChute);
        }
    }

    bool it_Equip2 = 0;
    AddTickol(Language::Translate("weapon.parachute.equipSecondary.label"), doesPedHavePara, it_Equip2, it_Equip2);
    if (it_Equip2)
    {
        if (DOES_ENTITY_EXIST(ped))
        {
            GivePedParachute(ped);
            SET_PLAYER_HAS_RESERVE_PARACHUTE(g_activePlayerId);
            //_0xAF04C87F5DC1DF38(Static_240, numChute2);
        }
    }

    bool it_RemoveWeapon = 0;
    AddOption(Language::Translate("weapon.parachute.remove.label"), it_RemoveWeapon);
    if (it_RemoveWeapon)
    {
        if (doesPedHavePara)
            REMOVE_WEAPON_FROM_PED(ped, whash);
    }

    bool goRgbSmokeMenu = 0;
    AddOption(Language::Translate("weapon.parachute.setSmokeColour.label"), goRgbSmokeMenu, nullFunc, SUB::MSPAINTS_RGB);
    if (Menu::IsLastDrawnOptionSelected())
        AddPresetColourOptionsPreviews(paraSmokeCol);
    if (goRgbSmokeMenu)
    {
        bitMSPaintsRGBMode = 7;
    }

    AddBreak(Language::Translate("weapon.parachute.tints"));

    bool tintPressed = 0;
    for (i = 0; i < vCaptions_ChuteTints.size(); i++)
    {
        tintPressed = 0;
        AddTickol(vCaptions_ChuteTints[i], currentChuteTint == i, tintPressed, tintPressed, TICKOL::WEAPONTHING);
        if (tintPressed)
        {
            SET_PLAYER_PARACHUTE_TINT_INDEX(player, i);
            if (IS_DLC_PRESENT(0xC40B8B70))
            {
                SET_PLAYER_PARACHUTE_MODEL_OVERRIDE(player, 0x73268708);
            }
            else if (IS_DLC_PRESENT(0x55292CC7))
            {
                SET_PLAYER_PARACHUTE_MODEL_OVERRIDE(player, 0x815E52EB);
            }
        }
    }
}

} // namespace WeaponIndivs_catind

namespace WeaponsLoadouts_catind
{
std::string& _searchStr = dict2;
std::string& _name = dict;
std::string& _dir = dict3;
auto& _ped = g_activePedHandle;

bool Create(GTAped ped, const std::string& filePath)
{
    pugi::xml_document doc;

    auto nodeDecleration = doc.append_child(pugi::node_declaration);
    nodeDecleration.append_attribute("version") = "1.0";
    nodeDecleration.append_attribute("encoding") = "ISO-8859-1";

    auto nodeLoadout = doc.append_child("Loadout"); // Root
    nodeLoadout.append_attribute("menyoo_ver") = MENYOO_VERSION_TEXT;

    std::vector<s_Weapon_Components_Tint> wct;
    ped.StoreWeaponsInArray(wct);

    for (auto& c : wct)
    {
        auto nodeWeapon = nodeLoadout.append_child("Weapon");
        nodeWeapon.append_attribute("hash") = IntToHexString(c.weaponHash, true).c_str();
        nodeWeapon.append_attribute("tint") = c.tint;
        auto nodeComponents = nodeWeapon.append_child("Components");
        for (auto& cc : c.componentHashes)
        {
            auto nodeComponent = nodeComponents.append_child("Component");
            nodeComponent.append_attribute("hash") = IntToHexString(cc.first, true).c_str();
            nodeComponent.append_attribute("livery") = IntToHexString(cc.second, true).c_str();
        }
    }

    return (doc.save_file((const char*)filePath.c_str()));
}

bool Apply(GTAped ped, const std::string& filePath)
{
    pugi::xml_document doc;
    if (doc.load_file((const char*)filePath.c_str()).status != pugi::status_ok)
        return false;

    auto nodeLoadout = doc.child("Loadout"); // Root

    std::vector<s_Weapon_Components_Tint> wct;
    for (auto nodeWeapon = nodeLoadout.child("Weapon"); nodeWeapon; nodeWeapon = nodeWeapon.next_sibling("Weapon"))
    {
        s_Weapon_Components_Tint c;
        c.weaponHash = nodeWeapon.attribute("hash").as_uint();
        c.tint = nodeWeapon.attribute("tint").as_int();
        auto nodeComponents = nodeWeapon.child("Components");
        for (auto nodeComponent = nodeComponents.first_child(); nodeComponent; nodeComponent = nodeComponent.next_sibling())
        {
            auto componentHashAttr = nodeComponent.attribute("hash");
            auto componentLiveryAttr = nodeComponent.attribute("livery");
            if (componentHashAttr)
                c.componentHashes.push_back({componentHashAttr.as_uint(), componentLiveryAttr ? componentLiveryAttr.as_int() : -1});
            else
                c.componentHashes.push_back({nodeComponent.text().as_uint(), componentLiveryAttr ? componentLiveryAttr.as_int() : -1});
        }
        wct.push_back(c);
    }

    if (NETWORK_IS_IN_SESSION())
        ped.RequestControl(600);
    ped.RemoveAllWeapons();
    ped.GiveWeaponsFromArray(wct);
    return true;
}

void Sub_Loadouts()
{
    bool save2 = false, bCreateFolderPressed = false;
    std::vector<std::string> vfilnames;

    AddTitle(Language::Translate("weapon.loadouts.title"));
    AddSaveFileSortOption();

    AddOption(Language::Translate("weapon.loadouts.saveCurrent.label"), save2);
    AddOptionDescription(Language::Translate("weapon.loadouts.saveCurrent.description"));

    AddOption(Language::Translate("weapon.loadouts.createFolder.label"), bCreateFolderPressed);

    //get_all_filenames_with_extension(GetPathffA(Pathff::WeaponsLoadout, false), ".xml", vfilnames, false);
    if (_dir.empty())
        _dir = GetPathffA(Pathff::WeaponsLoadout, false);
    DIR* dir_point = opendir(_dir.c_str());
    dirent* entry = readdir(dir_point);
    while (entry)
    {
        vfilnames.push_back(entry->d_name);
        entry = readdir(dir_point);
    }
    closedir(dir_point);
    SortSaveFileEntries(vfilnames, _dir);

    AddBreak(Language::Translate("weapon.loadouts.foundFiles.break"));

    bool bFolderBackPressed = false;
    AddOption("..", bFolderBackPressed);
    if (bFolderBackPressed)
    {
        _dir = _dir.substr(0, _dir.rfind("\\"));
        Menu::selectedOptionIndex = 5;
    }

    if (!vfilnames.empty())
    {
        bool bSearchPressed = false;
        AddOption(_searchStr.empty() ? Language::Translate("weapon.loadouts.search.label") : _searchStr, bSearchPressed, nullFunc, -1, true);
        if (bSearchPressed)
        {
            _searchStr = Game::InputBox(_searchStr, 126U, Language::Translate("weapon.loadouts.search.prompt"), boost::to_lower_copy(_searchStr));
            boost::to_upper(_searchStr);
            //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SearchToUpper, _searchStr, 126U, std::string(), _searchStr);
            //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&_searchStr);
        }

        for (auto& filname : vfilnames)
        {
            if (filname.front() == '.' || filname.front() == ',')
                continue;
            if (!_searchStr.empty())
            {
                if (boost::to_upper_copy(filname).find(_searchStr) == std::string::npos)
                    continue;
            }

            bool isFolder = PathIsDirectoryA((_dir + "\\" + filname).c_str()) != 0;
            bool isXml = filname.length() > 4 && filname.rfind(".xml") == filname.length() - 4;
            TICKOL icon = TICKOL::NONE;
            if (isFolder)
                icon = TICKOL::ARROWRIGHT;
            else if (isXml)
                icon = TICKOL::TICK2;
            bool bFilePressed = false;

            if (isFolder)
            {
                AddTickol(filname + " >>>", true, bFilePressed, bFilePressed, icon, TICKOL::NONE);
                if (bFilePressed)
                {
                    _dir = _dir + "\\" + filname;
                    Menu::selectedOptionIndex = 5;
                }

                if (Menu::IsLastDrawnOptionSelected() && !bFilePressed)
                {
                    if (FolderPreviewBmps_catind::bFolderBmpsEnabled)
                        FolderPreviewBmps_catind::DrawBmp(_dir + "\\" + filname);
                }
            }
            else if (isXml)
            {
                AddTickol(filname, true, bFilePressed, bFilePressed, icon, TICKOL::NONE);
                if (bFilePressed)
                {
                    _name = filname.substr(0, filname.rfind('.'));
                    Menu::pendingSubmenu = SUB::WEAPONOPS_LOADOUTS_INITEM;
                    return;
                }
            }
        }
    }

    if (save2)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("weapon.loadouts.loadoutName.prompt"));
        if (inputStr.length() > 0)
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("weapon.notifications.error.title"), Language::Translate("weapon.notifications.invalidName"));
            }
            else if (WeaponsLoadouts_catind::Create(_ped, _dir + "\\" + inputStr + ".xml"))
                Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.loadoutSaved"));
            else
                Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.unableToSaveLoadout"));
        }
        else
            Game::Print::PrintErrorInvalidInput(inputStr);
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SaveLoadoutToFile, std::string(), 28U, "Enter loadout name:");
        //OnscreenKeyboard::State::arg1._int = _ped;
        //OnscreenKeyboard::State::arg2._ptr = reinterpret_cast<std::string*>(&_dir);
    }

    if (bCreateFolderPressed)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("weapon.loadouts.folderName.prompt"));
        if (inputStr.length() > 0)
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("weapon.notifications.error.title"), Language::Translate("weapon.notifications.invalidName"));
            }
            else if (CreateDirectoryA((_dir + "\\" + inputStr).c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
            {
                _dir = _dir + "\\" + inputStr;
                Menu::selectedOptionIndex = 5;
                Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.folderCreated"));
            }
            else
            {
                Game::Print::ShowNotification(Language::Translate("weapon.notifications.failed.title"), Language::Translate("weapon.notifications.unableToCreateFolder"));
            }
        }
        else
            Game::Print::PrintErrorInvalidInput(inputStr);
        return;
        // No OnscreenKeyboard!
    }

    //if (Menu::selectedOptionIndex > Menu::currentOptionCount) Menu::Up();
    //else if (Menu::selectedOptionIndex <= 0) Menu::Down();
}

void Sub_Loadouts_InItem()
{
    std::string filePath = _dir + "\\" + _name + ".xml";

    bool bLoad = false, bRename = false, bOverwrite = false, bDelete = false;

    AddTitle(_name);
    AddOption(Language::Translate("weapon.loadouts.apply.label"), bLoad);
    AddOptionDescription(Language::Translate("weapon.loadouts.apply.description"));
    AddOption(Language::Translate("weapon.loadouts.renameFile.label"), bRename);
    AddOption(Language::Translate("weapon.loadouts.overwriteFile.label"), bOverwrite);
    AddOption(Language::Translate("weapon.loadouts.deleteFile.label"), bDelete);

    if (bLoad)
    {
        if (WeaponsLoadouts_catind::Apply(_ped, filePath))
            Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.loadoutApplied"));
        else
            Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.unableToApplyLoadout"));
    }

    if (bRename)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("weapon.loadouts.newName.prompt"), _name);
        if (inputStr.length())
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("weapon.notifications.error.title"), Language::Translate("weapon.notifications.invalidName"));
            }
            else if (rename(filePath.c_str(), (_dir + "\\" + inputStr + ".xml").c_str()) == 0)
            {
                _name = inputStr;
                Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.fileRenamed"));
            }
            else
                Game::Print::ShowNotification(Language::Translate("weapon.notifications.error.title"), Language::Translate("weapon.notifications.renamingFile"));
        }
        else
            Game::Print::PrintErrorInvalidInput(inputStr);
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::RenameLoadoutFile, std::string(), 28U, "Enter new name:");
        //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&_name);
        //OnscreenKeyboard::State::arg2._ptr = reinterpret_cast<void*>(&_dir);
    }

    if (bOverwrite)
    {
        if (WeaponsLoadouts_catind::Create(_ped, filePath))
            Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.fileOverwritten"));
        else
            Game::Print::ShowNotification(Language::Translate("weapon.notifications.error.title"), Language::Translate("weapon.notifications.unableToOverwriteFile"));
    }

    if (bDelete)
    {
        if (remove(filePath.c_str()) == 0)
            Game::Print::PrintBottomLeft(Language::Translate("weapon.notifications.fileDeleted"));
        else
            Game::Print::ShowNotification(Language::Translate("weapon.notifications.error.title"), Language::Translate("weapon.notifications.deletingFile"));
        Menu::SetPreviousMenu();
        Menu::Up();
        return;
    }

    // Attributes if any
}
} // namespace WeaponsLoadouts_catind

} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(WEAPONOPS, sub::Weaponops)
REGISTER_SUBMENU(WEAPONOPS_WEAPONFAVOURITES, sub::WeaponFavourites_catind::Sub_WeaponFavourites)
REGISTER_SUBMENU(WEAPONOPS_INDIVS_CATEGORIES, sub::WeaponIndivs_catind::Sub_CategoriesList)
REGISTER_SUBMENU(WEAPONOPS_INDIVS_CATEGORY, sub::WeaponIndivs_catind::Sub_InCategory)
REGISTER_SUBMENU(WEAPONOPS_INDIVS_ITEM, sub::WeaponIndivs_catind::Sub_InItem)
REGISTER_SUBMENU(WEAPONOPS_INDIVS_ITEM_MODS, sub::WeaponIndivs_catind::Sub_InItem_Mods)
REGISTER_SUBMENU(WEAPONOPS_LOADOUTS, sub::WeaponsLoadouts_catind::Sub_Loadouts)
REGISTER_SUBMENU(WEAPONOPS_LOADOUTS_INITEM, sub::WeaponsLoadouts_catind::Sub_Loadouts_InItem)
REGISTER_SUBMENU(WEAPONOPS_PARACHUTE, sub::WeaponIndivs_catind::Sub_Parachute)
REGISTER_SUBMENU(LIST_VEHICLECATS, sub::List_VehicleCats_Sub)
