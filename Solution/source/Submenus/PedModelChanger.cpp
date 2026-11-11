/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "PedModelChanger.h"
#include "..\Menu\Language.h"

#include "..\macros.h"

#include "..\Menu\Menu.h"
#include "..\Menu\Keybinds.h"
#include "..\Menu\Routine.h"
#include "..\Menu\StatsPanel.h"

#include "..\Natives\natives2.h"
#include "..\Scripting\Model.h"
#include "..\Util\ExePath.h"
#include "..\Util\FileLogger.h"
#include "..\Util\StringManip.h"
#include "..\Util\XmlDocumentCache.h"
#include "..\Util\keyboard.h"
#include "..\Scripting\Game.h"
#include "..\Scripting\ModelNames.h"
#include "..\Scripting\GTAped.h"
#include "..\Scripting\GTAvehicle.h"
#include "..\Scripting\Model.h"
#include "..\Scripting\PTFX.h"
#include "..\Scripting\WeaponIndivs.h"

#include "..\Submenus\Spooner\SpoonerEntity.h"
#include "..\Submenus\Spooner\Databases.h"
#include "..\Submenus\Spooner\EntityManagement.h"
#include "..\Submenus\Spooner\MenuOptions.h"
#include "Previews\Preview3DManager.h"

#include <string>
#include <vector>
#include <pugixml\src\pugixml.hpp>

namespace sub
{
namespace PedFavourites
{
std::string xmlFavouritePeds = "FavouritePeds.xml";
std::string searchStr = std::string();

namespace
{
Util::XmlDocumentCache favouritePedsCache;

bool SaveFavouritePeds(const pugi::xml_document& document)
{
    const bool saved = document.save_file((GetPathffA(Pathff::Main, true) + xmlFavouritePeds).c_str());
    favouritePedsCache.Invalidate();
    return saved;
}
} // namespace

void ClearSearchStr()
{
    searchStr.clear();
}

bool IsPedAFavourite(GTAmodel::Model model)
{
    const auto document = favouritePedsCache.Load(GetPathffA(Pathff::Main, true) + xmlFavouritePeds);
    const pugi::xml_node nodeRoot = document->document_element();
    return nodeRoot.find_child_by_attribute("hash", IntToHexString(model.hash, true).c_str()) != NULL;
}

bool AddPedToFavourites(GTAmodel::Model model, const std::string& customName)
{
    if (customName.empty())
    {
        return false;
    }
    pugi::xml_document document;
    if (document.load_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouritePeds).c_str()).status != pugi::status_ok)
    {
        document.reset();
        auto declaration = document.append_child(pugi::node_declaration);
        declaration.append_attribute("version") = "1.0";
        declaration.append_attribute("encoding") = "ISO-8859-1";
        document.append_child("FavouritePeds");
    }
    pugi::xml_node root = document.document_element();
    auto existingPed = root.find_child_by_attribute("hash", IntToHexString(model.hash, true).c_str());
    if (existingPed)
    {
        existingPed.parent().remove_child(existingPed);
    }
    auto pedNode = root.append_child("Ped");
    pedNode.append_attribute("hash") = IntToHexString(model.hash, true).c_str();
    pedNode.append_attribute("customName") = customName.c_str();
    return SaveFavouritePeds(document);
}

bool RemovePedFromFavourites(GTAmodel::Model model)
{
    pugi::xml_document document;
    if (document.load_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouritePeds).c_str()).status != pugi::status_ok)
    {
        return false;
    }
    pugi::xml_node root = document.document_element();
    auto existingPed = root.find_child_by_attribute("hash", IntToHexString(model.hash, true).c_str());
    if (existingPed)
    {
        existingPed.parent().remove_child(existingPed);
    }
    return SaveFavouritePeds(document);
}

void ShowInstructionalButton(GTAmodel::Model model)
{
    bool bIsAFav = IsPedAFavourite(model);
    Keybinds::AddBindIB(
        "menu_action", bIsAFav, Language::Translate("menu.player.ped_model.favourites.option.remove.label").c_str(), Language::Translate("menu.player.ped_model.favourites.option.add.label").c_str()
    );
    if (Keybinds::WasPressedThisFrame("menu_action"))
    {
        !bIsAFav ? AddPedToFavourites(model, Game::InputBox("", 28U, Language::Translate("menu.player.ped_model.input.option.custom_name.prompt").c_str(), GetPedModelLabel(model, true)))
                 : RemovePedFromFavourites(model);
    }
}

void PedFavouritesMenu()
{
    Menu::OnSubBack = ClearSearchStr;
    AddTitle(Language::Translate("menu.player.ped_model.favourites.title"));

    const auto document = favouritePedsCache.Load(GetPathffA(Pathff::Main, true) + xmlFavouritePeds);
    const pugi::xml_node root = document->document_element();

    bool inputAdd = false;
    AddOption(Language::Translate("menu.player.ped_model.favourites.option.add_new.label"), inputAdd);
    if (inputAdd)
    {
        std::string modelName = Game::InputBox("", 40U, Language::Translate("menu.player.ped_model.input.option.model_name.prompt").c_str());
        if (modelName.length())
        {
            Model model = GET_HASH_KEY(modelName);
            if (model.IsInCdImage())
            {
                WAIT(500);
                std::string customName = Game::InputBox("", 28U, Language::Translate("menu.player.ped_model.input.option.custom_name.prompt").c_str(), GetPedModelLabel(model, true));
                if (customName.length())
                {
                    if (AddPedToFavourites(model, customName))
                    {
                        Game::Print::PrintBottomLeft(Language::Translate("message.player.ped_model.added.message"));
                    }
                    else
                    {
                        Game::Print::PrintBottomLeft(Language::Translate("message.player.ped_model.add_failed.message"));
                    }
                }
                else
                {
                    Game::Print::PrintErrorInvalidInput(customName);
                }
            }
            else
            {
                Game::Print::PrintErrorInvalidModel(modelName);
            }
        }
        else
        {
            Game::Print::PrintErrorInvalidInput(modelName);
        }
    }

    if (root.first_child())
    {
        AddBreak(Language::Translate("menu.player.ped_model.favourites.section.added.label"));

        bool searchPressed = false;
        AddOption(searchStr.empty() ? Language::Translate("menu.player.ped_model.search.option.button.label") : searchStr, searchPressed, nullFunc, -1, true);
        if (searchPressed)
        {
            searchStr = Game::InputBox(searchStr, 126U, Language::Translate("menu.player.ped_model.option.search.prompt").c_str(), boost::to_lower_copy(searchStr));
            boost::to_upper(searchStr);
        }

        for (auto pedNode = root.first_child(); pedNode; pedNode = pedNode.next_sibling())
        {
            const std::string& customName = pedNode.attribute("customName").as_string();
            Model model = pedNode.attribute("hash").as_uint();

            if (!searchStr.empty())
            {
                if (boost::to_upper_copy(customName).find(searchStr) == std::string::npos)
                {
                    continue;
                }
            }
            AddModelOption(customName, model);
        }
    }
}
} // namespace PedFavourites

void ChangeModel(GTAmodel::Model model)
{
    if (model.IsInCdImage())
    {
        if (model.Load(4000))
        {
            GTAped playerPed = PLAYER_PED_ID();
            int oldPlayerPed = playerPed.Handle();

            playerPed.ForgetTrackedDamagePacks();
            playerPed.ForgetTrackedDecorations();

            std::vector<s_Weapon_Components_Tint> weaponsBackup;
            playerPed.StoreWeaponsInArray(weaponsBackup);
            Hash currWeaponHash = playerPed.GetWeapon();

            GTAentity att;
            auto spi = sub::Spooner::EntityManagement::GetEntityIndexInDb(playerPed);
            if (spi >= 0)
            {
                auto& spe = sub::Spooner::Databases::EntityDb[spi];
                sub::Spooner::EntityManagement::GetEntityThisEntityIsAttachedTo(spe.handle, att);
            }

            bool wasInVehicle = playerPed.IsInVehicle();
            GTAvehicle vehicle;
            VehicleSeat currentVehSeat;
            if (wasInVehicle)
            {
                vehicle = playerPed.CurrentVehicle();
                currentVehSeat = playerPed.GetCurrentVehicleSeat();
            }

            bool hasCollision = playerPed.GetIsCollisionEnabled();
            SET_PLAYER_MODEL(PLAYER_ID(), model.hash);

            playerPed = PLAYER_PED_ID();
            playerPed.SetIsCollisionEnabled(hasCollision);

            playerPed.SetDefaultComponentVariation();
            model.Unload();


            if (wasInVehicle)
            {
                playerPed.SetIntoVehicle(vehicle, currentVehSeat);
            }

            if (playerPed.PedType() == PedType::Animal && !HAS_ANIM_DICT_LOADED("creatures@rottweiler@melee@streamed_core@"))
            {
                REQUEST_ANIM_DICT("creatures@rottweiler@melee@streamed_core@");
                REQUEST_ANIM_DICT("creatures@cougar@melee@streamed_core@");
            }

            playerPed.GiveWeaponsFromArray(weaponsBackup);
            if (IS_WEAPON_VALID(currWeaponHash))
            {
                playerPed.SetWeapon(currWeaponHash);
            }

            SET_PED_INFINITE_AMMO_CLIP(playerPed.Handle(), bitInfiniteAmmo);

            if (spi >= 0)
            {
                auto& spe = sub::Spooner::Databases::EntityDb[spi];
                GTAentity oldPlayerPed = spe.handle;
                spe.handle = playerPed;
                spe.hashName = GetPedModelLabel(model, true);
                if (spe.hashName.length() == 0)
                {
                    IntToHexString(model.hash, true);
                }
                spe.ClearLastAnimations();
                spe.currentScenario.clear();
                if (att.Exists() && spe.attachmentArgs.isAttached)
                {
                    spe.handle.AttachTo(att, spe.attachmentArgs.boneIndex, spe.handle.GetIsCollisionEnabled(), spe.attachmentArgs.offset, spe.attachmentArgs.rotation);
                }
                spe.taskSequence.Reset();
                if (sub::Spooner::selectedEntity.handle.Equals(oldPlayerPed))
                {
                    sub::Spooner::selectedEntity = spe;
                }
            }
        }
    }
}

void AddModelChangerOption(const std::string& text, const GTAmodel::Model& model, int tickTrue)
{
    const GTAped& ped = Game::PlayerPed();
    const Model& pedModel = ped.Model();

    bool pressed = false;
    AddTickol(text, model.Equals(pedModel), pressed, pressed, static_cast<TICKOL>(tickTrue));
    if (pressed)
    {
        PTFX::TriggerPTFX("scr_solomon3", "scr_trev4_747_blood_impact", 0, ped.GetOffsetInWorldCoords(0.37, -0.32f, -1.32f), Vector3(90.0f, 0, 0), 0.7f);
        ChangeModel(model);
        addlog(ige::LogType::LOG_TRACE, "Changed model to: " + text);
    }
}

void AddModelOption(const std::string& text, const GTAmodel::Model& model, bool* extraOptionCode, int tickTrue)
{
    if (model.IsInCdImage())
    {
        bool isSpoonerSpawn = false;
        for (int ci = Menu::menuHistoryIndex; ci >= 0; ci--)
        {
            if (Menu::submenuHistory[ci] == SUB::SPOONER_SPAWN_PED)
            {
                isSpoonerSpawn = true;
                break;
            }
        }

        if (isSpoonerSpawn)
        {
            sub::Spooner::MenuOptions::AddOptionAddPed(text, model);
        }
        else
        {
            AddModelChangerOption(text, model.hash, tickTrue);
        }

        if (Menu::IsLastDrawnOptionSelected())
        {
            PedFavourites::ShowInstructionalButton(model);
            if (!isSpoonerSpawn && Previews::g_enabled)
            {
                const std::string previewKey = "ped-model:" + std::to_string(model.hash);
                std::vector<StatsPanel::Row> previewRows{Previews::CreateModelPreviewRow(previewKey, EntityType::PED, model)};
                StatsPanel::DrawAtOption(currentOptionY + menuPos.y, 0.100f, previewRows);
            }
        }
    }
}

std::pair<std::string, std::string> rngped;

void ModelChangerMenu()
{
    bool modelChangerRandomPedVariation = false;
    bool modelChangerInput = false;
    rngped = {"", ""};
    dict3.clear();

    g_activePedHandle = PLAYER_PED_ID();
    AddTitle(Language::Translate("menu.player.ped_model.title"));
    AddOption(Language::Translate("menu.player.ped_model.search.option.menu.label"), null, nullFunc, SUB::MODELCHANGER_SEARCH);
    AddOption(Language::Translate("menu.player.ped_model.favourites.option.menu.label"), null, nullFunc, SUB::MODELCHANGER_FAVOURITES);
    AddOption(Language::Translate("menu.player.ped_model.option.randomize_variation.label"), modelChangerRandomPedVariation);
    AddOptionDescription(Language::Translate("menu.player.ped_model.option.randomize_variation.description"));

    AddBreak(Language::Translate("menu.player.ped_model.section.categories.label"));
    AddOption(Language::Translate("menu.player.ped_model.category.option.player.label"), null, nullFunc, SUB::MODELCHANGER_PLAYER);
    AddOption(Language::Translate("menu.player.ped_model.category.option.story.label"), null, nullFunc, SUB::MODELCHANGER_STORY);
    AddOption(Language::Translate("menu.player.ped_model.category.option.cutscene.label"), null, nullFunc, SUB::MODELCHANGER_CS);
    AddOption(Language::Translate("menu.player.ped_model.category.option.multiplayer.label"), null, nullFunc, SUB::MODELCHANGER_MP);
    AddOption(Language::Translate("menu.player.ped_model.category.option.animals.label"), null, nullFunc, SUB::MODELCHANGER_ANIMAL);

    AddBreak(Language::Translate("menu.player.ped_model.section.ambient.label"));
    AddOption(Language::Translate("menu.player.ped_model.category.option.ambient_females.label"), null, nullFunc, SUB::MODELCHANGER_AMBFEMALES);
    AddOption(Language::Translate("menu.player.ped_model.category.option.ambient_males.label"), null, nullFunc, SUB::MODELCHANGER_AMBMALES);
    AddOption(Language::Translate("menu.player.ped_model.category.option.gang_female.label"), null, nullFunc, SUB::MODELCHANGER_GANGFEMALES);
    AddOption(Language::Translate("menu.player.ped_model.category.option.gang_males.label"), null, nullFunc, SUB::MODELCHANGER_GANGMALES);

    AddBreak(Language::Translate("menu.player.ped_model.section.scenario.label"));
    AddOption(Language::Translate("menu.player.ped_model.category.option.scenario_females.label"), null, nullFunc, SUB::MODELCHANGER_SCENARIOFEMALES);
    AddOptionDescription(Language::Translate("menu.player.ped_model.category.option.scenario.description"));
    AddOption(Language::Translate("menu.player.ped_model.category.option.scenario_males.label"), null, nullFunc, SUB::MODELCHANGER_SCENARIOMALES);
    AddOptionDescription(Language::Translate("menu.player.ped_model.category.option.scenario.description"));
    AddOption(Language::Translate("menu.player.ped_model.category.option.story_scenario_females.label"), null, nullFunc, SUB::MODELCHANGER_ST_SCENARIOFEMALES);
    AddOptionDescription(Language::Translate("menu.player.ped_model.category.option.story_scenario.description"));
    AddOption(Language::Translate("menu.player.ped_model.category.option.story_scenario_males.label"), null, nullFunc, SUB::MODELCHANGER_ST_SCENARIOMALES);
    AddOptionDescription(Language::Translate("menu.player.ped_model.category.option.story_scenario.description"));
    AddOption(Language::Translate("menu.player.ped_model.category.option.others.label"), null, nullFunc, SUB::MODELCHANGER_OTHERS);
    AddOption(Language::Translate("menu.player.ped_model.input.option.menu.label"), modelChangerInput);

    if (modelChangerRandomPedVariation)
    {
        addlog(ige::LogType::LOG_TRACE, "Random Ped Selected");
        GTAped activePed = g_activePedHandle;
        activePed.SetRandomComponentVariation();
        activePed.SetRandomProps();
        return;
    }

    if (modelChangerInput)
    {
        std::string inputStr = Game::InputBox("", 64U, Language::Translate("menu.player.ped_model.input.option.model_name.prompt").c_str());
        if (inputStr.length() > 0)
        {
            Model model = (inputStr);
            if (model.IsInCdImage())
            {
                ChangeModel(model);
            }
            else
            {
                Game::Print::PrintErrorInvalidModel(inputStr);
            }
            return;
        }
    }
}

GTAmodel::Model ModelChangerRandom(std::vector<std::pair<std::string, std::string>> pedModels)
{
    addlog(ige::LogType::LOG_TRACE, "Getting Random Ped Model");
    if (pedModels.empty())
        return 0;
    rngped = pedModels[std::rand() % pedModels.size()];
    addlog(ige::LogType::LOG_TRACE, "Got Random Ped Model: " + rngped.first + ", " + rngped.second);
    return rngped.first;
}

void ModelChangerPlayer()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.player.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_Player);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_Player)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerAnimal()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.animals.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_Animal);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_Animal)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerAmbientFemale()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.ambient_females.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_AmbientFemale);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_AmbientFemale)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerAmbientMale()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.ambient_males.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_AmbientMale);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_AmbientMale)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerCutscene()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.cutscene.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_Cutscene);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_Cutscene)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerGangFemale()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.gang_female.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_GangFemale);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_GangFemale)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerGangMale()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.gang_males.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_GangMale);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_GangMale)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerStory()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.story.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_Story);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_Story)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerMultiplayer()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.multiplayer.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_Multiplayer);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_Multiplayer)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerScenarioFemale()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.scenario_females.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_ScenarioFemale);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_ScenarioFemale)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerScenarioMale()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.scenario_males.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_ScenarioMale);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_ScenarioMale)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerStoryScenarioFemale()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.story_scenario_females.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_StoryScenarioFemale);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_StoryScenarioFemale)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerStoryScenarioMale()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.story_scenario_males.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_StoryScenarioMale);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_StoryScenarioMale)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

void ModelChangerOthers()
{
    AddTitle(Language::Translate("menu.player.ped_model.category.others.title"));
    if (rngped.first == Game::PlayerPed().Model() || rngped.first == "")
    {
        ModelChangerRandom(g_pedModels_Others);
    }
    AddModelOption(Language::Translate("menu.player.ped_model.option.random.label"), rngped.first, nullptr, 0);
    for (auto& pmn : g_pedModels_Others)
    {
        AddModelOption(pmn.second, (pmn.first));
    }
}

namespace PedSearch
{
static int sortIndex = 0;
static int categoryFilter = 0;
static std::vector<std::pair<std::string, std::string>> results;
static std::vector<std::string> categoryOptions;
static bool dirty = true;
static std::string lastSearch;
static int lastSort = -1, lastCategory = -1;

static const std::vector<std::string> sortOptionKeys = {
    "menu.player.ped_model.search.option.sort_name_asc.label", "menu.player.ped_model.search.option.sort_name_desc.label", "menu.player.ped_model.search.option.sort_model.label"
};

struct CategoryEntry
{
    const char* name;
    std::vector<std::pair<std::string, std::string>>* vec;
};

static const CategoryEntry categories[] = {
    {"Player", &g_pedModels_Player},
    {"Animals", &g_pedModels_Animal},
    {"Ambient Females", &g_pedModels_AmbientFemale},
    {"Ambient Males", &g_pedModels_AmbientMale},
    {"Cutscene", &g_pedModels_Cutscene},
    {"Gang Females", &g_pedModels_GangFemale},
    {"Gang Males", &g_pedModels_GangMale},
    {"Story", &g_pedModels_Story},
    {"Multiplayer", &g_pedModels_Multiplayer},
    {"Scenario Females", &g_pedModels_ScenarioFemale},
    {"Scenario Males", &g_pedModels_ScenarioMale},
    {"Story Scenario F", &g_pedModels_StoryScenarioFemale},
    {"Story Scenario M", &g_pedModels_StoryScenarioMale},
    {"Others", &g_pedModels_Others},
};
static const int numCategories = 14;
static const std::vector<std::string> categoryKeys = {
    "menu.player.ped_model.category.option.player.label",
    "menu.player.ped_model.category.option.animals.label",
    "menu.player.ped_model.category.option.ambient_females.label",
    "menu.player.ped_model.category.option.ambient_males.label",
    "menu.player.ped_model.category.option.cutscene.label",
    "menu.player.ped_model.search.option.category.value.gang_females",
    "menu.player.ped_model.category.option.gang_males.label",
    "menu.player.ped_model.category.option.story.label",
    "menu.player.ped_model.category.option.multiplayer.label",
    "menu.player.ped_model.category.option.scenario_females.label",
    "menu.player.ped_model.category.option.scenario_males.label",
    "menu.player.ped_model.category.option.story_scenario_females.label",
    "menu.player.ped_model.category.option.story_scenario_males.label",
    "menu.player.ped_model.category.option.others.label"
};

void BuildCategoryOptions()
{
    categoryOptions.clear();
    categoryOptions.push_back(
        Language::Format(
            "menu.player.ped_model.search.option.category.value.count", {{"category", Language::Translate("menu.player.ped_model.search.option.all.label")}, {"count", g_pedModels.size()}}
        )
    );
    for (int i = 0; i < numCategories; i++)
    {
        categoryOptions.push_back(
            Language::Format("menu.player.ped_model.search.option.category.value.count", {{"category", Language::Translate(categoryKeys[i])}, {"count", categories[i].vec->size()}})
        );
    }
}

void RebuildResults(const std::string& searchStr)
{
    results.clear();
    std::string searchUpper = boost::to_upper_copy(searchStr);

    auto* sourceVec = &g_pedModels;
    if (categoryFilter > 0 && categoryFilter <= numCategories)
        sourceVec = categories[categoryFilter - 1].vec;

    for (auto& ped : *sourceVec)
    {
        if (!searchUpper.empty())
        {
            std::string nameUpper = boost::to_upper_copy(ped.second);
            std::string modelUpper = boost::to_upper_copy(ped.first);
            if (nameUpper.find(searchUpper) == std::string::npos && modelUpper.find(searchUpper) == std::string::npos)
                continue;
        }
        results.push_back(ped);
    }

    switch (sortIndex)
    {
    case 0:
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return boost::to_lower_copy(a.second) < boost::to_lower_copy(b.second); });
        break;
    case 1:
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return boost::to_lower_copy(a.second) > boost::to_lower_copy(b.second); });
        break;
    case 2:
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return boost::to_lower_copy(a.first) < boost::to_lower_copy(b.first); });
        break;
    }
}
} // namespace PedSearch

void ModelChangerSearchMenu()
{
    using namespace PedSearch;
    auto& searchStr = dict3;

    if (categoryOptions.empty())
        BuildCategoryOptions();

    AddTitle(Language::Translate("menu.player.ped_model.search.title"));

    bool bSearchPressed = false;
    AddOption(searchStr.empty() ? Language::Translate("menu.player.ped_model.search.option.button.label") : ("~b~" + searchStr + "~s~"), bSearchPressed, nullFunc, -1, true);
    if (bSearchPressed)
    {
        searchStr = Game::InputBox(searchStr, 64U, Language::Translate("menu.player.ped_model.option.search.prompt").c_str(), boost::to_lower_copy(searchStr));
        boost::to_upper(searchStr);
    }

    bool sortRight = false, sortLeft = false;
    const std::vector<std::string> sortOptions{Language::Translate(sortOptionKeys[0]), Language::Translate(sortOptionKeys[1]), Language::Translate(sortOptionKeys[2])};
    AddTexter(Language::Translate("menu.player.ped_model.search.option.sort.label"), sortIndex, sortOptions, null, sortRight, sortLeft);
    if (sortRight && sortIndex < (int)(sortOptions.size() - 1))
        sortIndex++;
    if (sortLeft && sortIndex > 0)
        sortIndex--;

    bool catRight = false, catLeft = false;
    AddTexter(Language::Translate("menu.player.ped_model.search.option.category.label"), categoryFilter, categoryOptions, null, catRight, catLeft);
    if (catRight && categoryFilter < (int)(categoryOptions.size() - 1))
        categoryFilter++;
    if (catLeft && categoryFilter > 0)
        categoryFilter--;

    if (dirty || searchStr != lastSearch || sortIndex != lastSort || categoryFilter != lastCategory)
    {
        RebuildResults(searchStr);
        lastSearch = searchStr;
        lastSort = sortIndex;
        lastCategory = categoryFilter;
        dirty = false;
    }

    AddBreak(Language::Format("menu.player.ped_model.search.section.results.label", {{"count", results.size()}}));

    for (auto& ped : results)
    {
        AddModelOption(ped.second, (ped.first));
    }
}

} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(MODELCHANGER, sub::ModelChangerMenu)
REGISTER_SUBMENU(MODELCHANGER_FAVOURITES, sub::PedFavourites::PedFavouritesMenu)
REGISTER_SUBMENU(MODELCHANGER_PLAYER, sub::ModelChangerPlayer)
REGISTER_SUBMENU(MODELCHANGER_ANIMAL, sub::ModelChangerAnimal)
REGISTER_SUBMENU(MODELCHANGER_AMBFEMALES, sub::ModelChangerAmbientFemale)
REGISTER_SUBMENU(MODELCHANGER_AMBMALES, sub::ModelChangerAmbientMale)
REGISTER_SUBMENU(MODELCHANGER_CS, sub::ModelChangerCutscene)
REGISTER_SUBMENU(MODELCHANGER_GANGFEMALES, sub::ModelChangerGangFemale)
REGISTER_SUBMENU(MODELCHANGER_GANGMALES, sub::ModelChangerGangMale)
REGISTER_SUBMENU(MODELCHANGER_STORY, sub::ModelChangerStory)
REGISTER_SUBMENU(MODELCHANGER_MP, sub::ModelChangerMultiplayer)
REGISTER_SUBMENU(MODELCHANGER_SCENARIOFEMALES, sub::ModelChangerScenarioFemale)
REGISTER_SUBMENU(MODELCHANGER_SCENARIOMALES, sub::ModelChangerScenarioMale)
REGISTER_SUBMENU(MODELCHANGER_ST_SCENARIOFEMALES, sub::ModelChangerStoryScenarioFemale)
REGISTER_SUBMENU(MODELCHANGER_ST_SCENARIOMALES, sub::ModelChangerStoryScenarioMale)
REGISTER_SUBMENU(MODELCHANGER_OTHERS, sub::ModelChangerOthers)
REGISTER_SUBMENU(MODELCHANGER_SEARCH, sub::ModelChangerSearchMenu)
