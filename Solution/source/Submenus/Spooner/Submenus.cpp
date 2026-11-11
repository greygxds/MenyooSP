/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "Submenus.h"
#include "..\..\Menu\Language.h"

#include "..\..\macros.h"

#include "..\..\Menu\Menu.h"
#include "..\..\Menu\Keybinds.h"
#include "..\..\Menu\Routine.h"

#include "..\..\Natives\natives2.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\GTAprop.h"
#include "..\..\Scripting\GTAvehicle.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Util\GTAmath.h"
#include "..\..\Scripting\ModelNames.h"
#include "..\..\Scripting\Game.h"
#include "..\..\Scripting\World.h"
#include "..\..\Memory\GTAmemory.h"
#include "..\..\Util\ExePath.h"
#include "..\..\Util\StringManip.h"
#include "..\..\Menu\FolderPreviewBmps.h"
#include "..\..\Menu\MenuCategory.h"
#include "..\..\Menu\StatsPanel.h"
#include "..\..\Scripting\DxHookIMG.h"
#include "..\..\Scripting\GTAblip.h"
#include "..\..\Scripting\TimecycleModification.h"
#include "..\..\Scripting\Camera.h"
#include "..\..\Util\keyboard.h"
#include "..\..\Scripting\WeaponIndivs.h"
#include "..\Previews\Preview3DManager.h"

#include "STSTask.h"
#include "STSTasks.h"
#include "SpoonerEntity.h"
#include "SpoonerMode.h"
#include "SpoonerSettings.h"
#include "Databases.h"
#include "FileManagement.h"
#include "EntityManagement.h"
#include "MarkerManagement.h"
#include "FavouritesManagement.h"
#include "MenuOptions.h"
#include "SpoonerMarker.h"
#include "SpoonerLight.h"
#include "..\..\Submenus\Settings.h"
#include "..\..\Submenus\PedModelChanger.h"
#include "..\..\Submenus\VehicleSpawner.h"
#include "..\..\Submenus\VehicleModShop.h"
#include "..\..\Submenus\PedAnimation.h"
#include "..\..\Submenus\PedSpeech.h"
#include "..\..\Submenus\PtfxSubs.h"
#include "..\..\Util\FileLogger.h"
#include "..\..\Util\ObjectCategories.h"

#include "BlipCustoms.h"
#include "BlipMapping.h"
#include "SpoonerBlips.h"

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <cmath>
#include <string>
#include <fstream>
#include <tuple>
#include <utility>
#include <vector>
#include <set>
#include <array>
#include <pugixml/src/pugixml.hpp>
#include <dirent\include\dirent.h>

namespace sub
{
namespace Spooner::Submenus
{
std::string& _searchStr = dict2;
std::tuple<GTAentity, Vector3*, Vector3*> SpoonerVector3ManualEditingPtrs = {0, nullptr, nullptr};
float _fSaveRangeRadius = 5.0f;
UINT8 _copyEntTexterValue = 0;
UINT8 _entTypeToShowTexterValue = 0;
EntityScaleState _vehScale, _pedScale, _objScale;
static GTAentity s_selectedEntityTarget;
static UINT s_spoonerPedWeaponCategory = 0;

struct FavouriteProp
{
    std::string modelName;
    Model model;
    std::string category;
};

struct FavPropCache
{
    std::map<std::string, std::vector<FavouriteProp>> byCategory;
    std::vector<std::string> sortedCategories;
    bool needsRebuild = true;
};

static FavPropCache s_favPropCache;
static std::string s_favPropSearchStr;

namespace MultiSelect
{
std::vector<SpoonerEntity> MultiSelect::g_selectedEntities;
std::unordered_map<int, EntityState> MultiSelect::g_savedEntityStates;
bool MultiSelect::g_bulkEditActive = false;
SpoonerEntity MultiSelect::g_prevSelected;
GTAentity MultiSelect::g_groupPivot;

void SaveEntityState(const SpoonerEntity& entity)
{
    if (!entity.handle.Exists())
        return;
    int h = entity.handle.GetHandle();
    if (g_savedEntityStates.find(h) != g_savedEntityStates.end())
        return; // keep the pre-attach state
    g_savedEntityStates[h] = {entity.handle.GetIsCollisionEnabled(), entity.handle.IsPositionFrozen(), entity.dynamic};
}

void DiscardEntityState(GTAentity handle)
{
    g_savedEntityStates.erase(handle.GetHandle());
}

static void ApplyEntityState(GTAentity handle, const EntityState& s)
{
    handle.SetIsCollisionEnabled(s.collision);
    handle.FreezePosition(s.frozen);
    handle.SetDynamic(s.dynamic);
}

static void SyncDynamicFlag(GTAentity handle, bool dynamic)
{
    int idx = EntityManagement::GetEntityIndexInDb(handle);
    if (idx >= 0)
        Databases::EntityDb[idx].dynamic = dynamic;
}

void RestoreEntityState(SpoonerEntity& entity)
{
    auto it = g_savedEntityStates.find(entity.handle.GetHandle());
    if (it == g_savedEntityStates.end())
        return;
    EntityState s = it->second;
    g_savedEntityStates.erase(it);
    if (!entity.handle.Exists())
        return;
    entity.handle.Detach();
    ApplyEntityState(entity.handle, s);
    entity.dynamic = s.dynamic;
    SyncDynamicFlag(entity.handle, s.dynamic);
}

void RestoreAllEntityStates()
{
    for (auto& e : MultiSelect::g_selectedEntities)
        RestoreEntityState(e);
}

void ApplySavedEntityState(GTAentity origHandle, GTAentity newHandle)
{
    if (!newHandle.Exists())
        return;
    auto it = g_savedEntityStates.find(origHandle.GetHandle());
    if (it == g_savedEntityStates.end())
        return;
    ApplyEntityState(newHandle, it->second);
    SyncDynamicFlag(newHandle, it->second.dynamic);
}

void Add(const SpoonerEntity& entity)
{
    if (!entity.handle.Exists())
        return;
    if (IsSelected(entity.handle))
        return;
    // attached entities can't be selected, pivot would replace their attachment
    if (entity.handle.IsAttached())
        return;
    MultiSelect::g_selectedEntities.push_back(entity);
}

void Remove(int index)
{
    if (index < 0 || index >= static_cast<int>(MultiSelect::g_selectedEntities.size()))
        return;
    RestoreEntityState(MultiSelect::g_selectedEntities[index]);
    MultiSelect::g_selectedEntities.erase(MultiSelect::g_selectedEntities.begin() + index);
}

void Remove(GTAentity handle)
{
    for (size_t i = 0; i < MultiSelect::g_selectedEntities.size(); i++)
    {
        if (MultiSelect::g_selectedEntities[i].handle == handle)
        {
            RestoreEntityState(MultiSelect::g_selectedEntities[i]);
            MultiSelect::g_selectedEntities.erase(MultiSelect::g_selectedEntities.begin() + i);
            return;
        }
    }
    DiscardEntityState(handle);
}

bool IsSelected(GTAentity handle)
{
    for (auto& e : MultiSelect::g_selectedEntities)
    {
        if (e.handle == handle)
            return true;
    }
    return false;
}

void SelectAll()
{
    for (auto& e : Databases::EntityDb)
    {
        if (e.handle.Exists())
            Add(e);
    }
}

void Clear()
{
    RestoreAllEntityStates();
    MultiSelect::g_selectedEntities.clear();
    MultiSelect::g_savedEntityStates.clear();
}

void DestroyPivot()
{
    if (MultiSelect::g_groupPivot.Exists())
    {
        RestoreAllEntityStates();
        MultiSelect::g_groupPivot.Delete();
    }
}

void CreatePivot()
{
    Vector3 centroid;
    int count = 0;
    for (auto& e : MultiSelect::g_selectedEntities)
    {
        if (e.handle.Exists())
        {
            centroid = centroid + e.handle.GetPosition();
            count++;
        }
    }
    if (count == 0)
        return;

    centroid /= static_cast<float>(count);
    GTAprop prop = World::CreateProp(GTAmodel::Model(0x3A49EBD1), centroid, Vector3(), false, false);
    MultiSelect::g_groupPivot = GTAentity(prop.Handle());
    MultiSelect::g_groupPivot.SetAlpha(0);
    SET_ENTITY_COLLISION(MultiSelect::g_groupPivot.Handle(), false, false);
    MultiSelect::g_groupPivot.FreezePosition(true);

    Vector3 pivotPos = MultiSelect::g_groupPivot.GetPosition();
    Vector3 pivotRot = MultiSelect::g_groupPivot.GetRotation();
    auto attachEntityToPivot = [&](SpoonerEntity& e)
    {
        if (!e.handle.Exists())
            return;
        MultiSelect::SaveEntityState(e);
        Vector3 relPos = e.handle.GetPosition() - pivotPos;
        Vector3 relRot = e.handle.GetRotation() - pivotRot;
        e.handle.AttachTo(MultiSelect::g_groupPivot, -1, false, relPos, relRot);
    };
    // entities split into two groups: peds and non-peds
    // this is due to a bug where attaching a ped after an entity that it was previously attached to caused a crash (weird GTA quirk?), so we just attach peds first
    for (auto& e : MultiSelect::g_selectedEntities)
        if (e.handle.IsPed())
            attachEntityToPivot(e);
    for (auto& e : MultiSelect::g_selectedEntities)
        if (!e.handle.IsPed())
            attachEntityToPivot(e);
}
} // namespace MultiSelect

void SetSelectedEntityAsActivePed()
{
    g_activePedHandle = selectedEntity.handle.Handle();
}

void SetSelectedEntityAsVehicleTarget()
{
    s_selectedEntityTarget = selectedEntity.handle;
    SetVehicleModShopTarget(selectedEntity.handle.Handle());
}

void SetPlayerAsEntityAlphaTarget()
{
    s_selectedEntityTarget = PLAYER_PED_ID();
}

void Sub_SpoonerMain()
{
    SpoonerMode::editingState.SetMode(SpoonerMode::eEditMode::Disabled);
    selectedEntity.handle = 0;
    _searchStr.clear(); // Sub_SaveFiles _searchStr
    dict3.clear();      // Sub_SaveFiles _dir

    AddTitle(Language::Translate("menu.spooner.main.title"));

    AddLocal(Language::Translate("menu.spooner.main.option.mode.label"), SpoonerMode::bEnabled, SpoonerMode::Toggle, SpoonerMode::Toggle);
    AddOptionDescription(Language::Translate("menu.spooner.main.option.mode.description"));
    AddOption(Language::Translate("menu.spooner.main.option.spawn.label"), null, nullFunc, SUB::SPOONER_SPAWN_CATEGORIES);
    AddOption(Language::Translate("menu.spooner.main.option.database.label"), null, nullFunc, SUB::SPOONER_MANAGEDB);
    AddOption(Language::Translate("menu.spooner.main.option.multiple.label"), null, nullFunc, SUB::SPOONER_MULTISELECT);
    AddOptionDescription(Language::Translate("menu.spooner.main.option.multiple.description"));
    AddOption(Language::Translate("menu.spooner.main.option.markers.label"), null, nullFunc, SUB::SPOONER_MANAGEMARKERS);
    AddOption(Language::Translate("menu.spooner.main.option.lights.label"), null, nullFunc, SUB::SPOONER_MANAGELIGHTS);
    AddOption(Language::Translate("menu.spooner.main.option.blips.label"), null, nullFunc, SUB::SPOONER_BLIPS);
    AddOption(Language::Translate("menu.spooner.main.option.markers.label"), null, nullFunc, SUB::SPOONER_MANAGEMARKERS);
    AddOption(Language::Translate("menu.spooner.main.option.saved_files.label"), null, nullFunc, SUB::SPOONER_SAVEFILES);
    AddOption(Language::Translate("menu.spooner.main.option.jobs.label"), null, nullFunc, SUB::SPOONER_JOBIMPORTER);
    AddOption(Language::Translate("menu.spooner.main.option.settings.label"), null, nullFunc, SUB::SPOONER_SETTINGS);
}

void Sub_Settings()
{
    AddTitle(Language::Translate("menu.spooner.settings.title"));
    AddToggle(Language::Translate("menu.spooner.settings.option.previews.label"), Settings::bShowModelPreviews);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.previews.description"));
    AddNumberStepper(Language::Translate("menu.spooner.settings.option.rotation_step.label"), Settings::previewRotateStepDegrees, 1, 0.5, 0.5, 45.0);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.rotation_step.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.info.label"), Settings::bDisplaySpoonerInfo);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.info.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.box.label"), Settings::bShowBoxAroundSelectedEntity);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.box.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.dynamic_objects.label"), Settings::bSpawnDynamicProps);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.dynamic_objects.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.dynamic_peds.label"), Settings::bSpawnDynamicPeds);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.dynamic_peds.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.dynamic_vehicles.label"), Settings::bSpawnDynamicVehicles);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.dynamic_vehicles.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.freeze_moving.label"), Settings::bFreezeEntityWhenMovingIt);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.freeze_moving.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.invincible.label"), Settings::bSpawnInvincibleEntities);
    AddToggle(Language::Translate("menu.spooner.settings.option.still_peds.label"), Settings::bSpawnStillPeds);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.still_peds.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.persistent.label"), Settings::bAddToDbAsMissionEntities);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.persistent.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.teleport_reference.label"), Settings::bTeleportToReferenceWhenLoadingFile);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.teleport_reference.description"));
    AddToggle(Language::Translate("menu.spooner.settings.option.invert_scroll.label"), Settings::bInvertScrollSensitivity);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.invert_scroll.description"));

    AddOption(Language::Translate("menu.spooner.settings.option.reload_models.label"), null, PopulateGlobalEntityModelsArrays);
    AddOptionDescription(Language::Translate("menu.spooner.settings.option.reload_models.description"));

    AddOption(Language::Translate("menu.spooner.settings.option.grid_snap.label"), null, nullFunc, SUB::SPOONER_MANUALEDITING_SNAP);
}

static void DrawSaveFileStats(const std::string& filePath, float optionTextY)
{
    static std::string lastStatsPath = "";
    static std::string lastStatsLocale = "";
    static std::vector<StatsPanel::Row> statsRows;
    static std::string lastHoveredXml = "";
    static DxHookIMG::DxTexture hoveredXmlTexture;

    std::string hoverImgPath = filePath.substr(0, filePath.rfind('.')) + ".jpg";
    if (lastHoveredXml != hoverImgPath)
    {
        lastHoveredXml = hoverImgPath;
        hoveredXmlTexture = DxHookIMG::DxTexture();
        std::ifstream f(hoverImgPath);
        if (f.good())
            hoveredXmlTexture.Load(hoverImgPath);
    }

    const std::string locale = Language::GetSelectedLocale();
    if (lastStatsPath != filePath || lastStatsLocale != locale)
    {
        lastStatsPath = filePath;
        lastStatsLocale = locale;
        statsRows.clear();

        int entityCount = 0, pedCount = 0, vehCount = 0, propCount = 0, markerCount = 0, blipCount = 0, lightCount = 0, iplCount = 0;
        std::string saveNote = "";
        std::string audioFile = "";
        std::string weatherToSet = "";
        std::string timecycleMod = "";
        bool clearDatabase = false;
        bool clearMarkers = false;
        bool startTaskSeqs = true;
        float clearWorldRadius = 0.0f;
        float centroidX = 0.0f, centroidY = 0.0f, centroidZ = 0.0f;
        int placementCount = 0;
        float refX = 0.0f, refY = 0.0f, refZ = 0.0f;
        bool haveRefCoords = false;

        pugi::xml_document doc;
        if (doc.load_file((const char*)filePath.c_str()).status == pugi::status_ok)
        {
            auto nodeRoot = doc.child("SpoonerPlacements");
            saveNote = nodeRoot.child("Note").text().as_string();
            audioFile = nodeRoot.child("AudioFile").text().as_string();
            weatherToSet = nodeRoot.child("WeatherToSet").text().as_string();
            timecycleMod = nodeRoot.child("TimecycleModifier").text().as_string();
            clearDatabase = nodeRoot.child("ClearDatabase").text().as_bool();
            clearMarkers = nodeRoot.child("ClearMarkers").text().as_bool();
            auto nodeStartTaskSeqs = nodeRoot.child("StartTaskSequencesOnLoad");
            if (nodeStartTaskSeqs)
                startTaskSeqs = nodeStartTaskSeqs.text().as_bool(true);
            clearWorldRadius = nodeRoot.child("ClearWorld").text().as_float();
            auto nodeReferenceCoords = nodeRoot.child("ReferenceCoords");
            if (nodeReferenceCoords)
            {
                refX = nodeReferenceCoords.child("X").text().as_float();
                refY = nodeReferenceCoords.child("Y").text().as_float();
                refZ = nodeReferenceCoords.child("Z").text().as_float();
                haveRefCoords = true;
            }
            for (auto node = nodeRoot.first_child(); node; node = node.next_sibling())
            {
                std::string name = node.name();
                if (name == "Placement")
                {
                    entityCount++;
                    auto nodePosRot = node.child("PositionRotation");
                    if (nodePosRot)
                    {
                        centroidX += nodePosRot.child("X").text().as_float();
                        centroidY += nodePosRot.child("Y").text().as_float();
                        centroidZ += nodePosRot.child("Z").text().as_float();
                        placementCount++;
                    }
                    switch ((EntityType)node.child("Type").text().as_int())
                    {
                    case EntityType::PED:
                        pedCount++;
                        break;
                    case EntityType::VEHICLE:
                        vehCount++;
                        break;
                    case EntityType::PROP:
                        propCount++;
                        break;
                    default:
                        break;
                    }
                }
                else if (name == "Marker")
                    markerCount++;
                else if (name == "Blip")
                    blipCount++;
                else if (name == "Light")
                    lightCount++;
                else if (name == "IPLsToLoad")
                    iplCount++;
            }
        }

        Vector3 locationPos(0.0f, 0.0f, 0.0f);
        bool haveLocation = false;
        if (haveRefCoords)
        {
            locationPos = Vector3(refX, refY, refZ);
            haveLocation = true;
        }
        else if (placementCount > 0)
        {
            locationPos = Vector3(centroidX / placementCount, centroidY / placementCount, centroidZ / placementCount);
            haveLocation = true;
        }
        if (haveLocation)
        {
            std::string zoneName = World::GetZoneName(locationPos, true);
            if (!zoneName.empty())
                statsRows.push_back(StatsPanel::StatRow{Language::Translate("component.spooner_file_preview.location.label"), zoneName});
            else
                statsRows.push_back(
                    StatsPanel::StatRow{
                        Language::Translate("component.spooner_file_preview.location.label"),
                        std::to_string((int)std::round(locationPos.x)) + ", " + std::to_string((int)std::round(locationPos.y)) + ", " + std::to_string((int)std::round(locationPos.z))
                    }
                );
        }

        statsRows.push_back(
            StatsPanel::StatRow{Language::Format("component.spooner_file_preview.count.entities.label", {{"count", static_cast<std::uint64_t>(entityCount)}}), std::to_string(entityCount)}
        );
        if (pedCount)
            statsRows.push_back(StatsPanel::StatRow{Language::Format("component.spooner_file_preview.count.peds.label", {{"count", static_cast<std::uint64_t>(pedCount)}}), std::to_string(pedCount)});
        if (vehCount)
            statsRows.push_back(
                StatsPanel::StatRow{Language::Format("component.spooner_file_preview.count.vehicles.label", {{"count", static_cast<std::uint64_t>(vehCount)}}), std::to_string(vehCount)}
            );
        if (propCount)
            statsRows.push_back(
                StatsPanel::StatRow{Language::Format("component.spooner_file_preview.count.objects.label", {{"count", static_cast<std::uint64_t>(propCount)}}), std::to_string(propCount)}
            );
        if (markerCount)
            statsRows.push_back(
                StatsPanel::StatRow{Language::Format("component.spooner_file_preview.count.markers.label", {{"count", static_cast<std::uint64_t>(markerCount)}}), std::to_string(markerCount)}
            );
        if (blipCount)
            statsRows.push_back(
                StatsPanel::StatRow{Language::Format("component.spooner_file_preview.count.blips.label", {{"count", static_cast<std::uint64_t>(blipCount)}}), std::to_string(blipCount)}
            );
        if (lightCount)
            statsRows.push_back(
                StatsPanel::StatRow{Language::Format("component.spooner_file_preview.count.lights.label", {{"count", static_cast<std::uint64_t>(lightCount)}}), std::to_string(lightCount)}
            );
        if (iplCount)
            statsRows.push_back(StatsPanel::StatRow{Language::Format("component.spooner_file_preview.count.ipls.label", {{"count", static_cast<std::uint64_t>(iplCount)}}), std::to_string(iplCount)});
        if (!audioFile.empty())
            statsRows.push_back(StatsPanel::StatRow{Language::Translate("component.spooner_file_preview.audio.label"), audioFile});
        if (!weatherToSet.empty())
            statsRows.push_back(StatsPanel::StatRow{Language::Translate("component.spooner_file_preview.weather.label"), weatherToSet});
        if (!timecycleMod.empty())
            statsRows.push_back(StatsPanel::StatRow{Language::Translate("component.spooner_file_preview.timecycle.label"), timecycleMod});
        if (clearDatabase)
            statsRows.push_back(StatsPanel::StatRow{Language::Translate("component.spooner_file_preview.clear_database.label"), Language::Translate("menu.stats_panel.option.yes.label")});
        if (clearMarkers)
            statsRows.push_back(StatsPanel::StatRow{Language::Translate("component.spooner_file_preview.clear_markers.label"), Language::Translate("menu.stats_panel.option.yes.label")});
        if (clearWorldRadius > 0.0f)
            statsRows.push_back(StatsPanel::StatRow{Language::Translate("component.spooner_file_preview.clear_area.label"), std::to_string((int)clearWorldRadius) + "m"});
        if (!startTaskSeqs)
            statsRows.push_back(StatsPanel::StatRow{Language::Translate("component.spooner_file_preview.task_sequences.label"), Language::Translate("menu.stats_panel.option.off.label")});
        statsRows.push_back(StatsPanel::SeparatorRow{});
        statsRows.push_back(StatsPanel::StatRow{Language::Translate("menu.stats_panel.option.size.label"), GetFileSizeStr(filePath)});
        statsRows.push_back(StatsPanel::StatRow{Language::Translate("menu.stats_panel.option.modified.label"), GetFileLastWriteDateStr(filePath)});
        for (auto& noteLine : StatsPanel::WrapText(saveNote, 24))
            statsRows.push_back(StatsPanel::TextRow{noteLine});
    }

    if (Previews::g_enabled || hoveredXmlTexture.Exists())
    {
        std::vector<StatsPanel::Row> rowsWithImage;
        rowsWithImage.push_back(
            StatsPanel::Preview3DRow{
                [&, filePath](float centerX, float centerY, float width, float height)
                {
                    const bool previewDrawn = Previews::g_enabled && Previews::DrawSpoonerScenePreview(filePath, centerX, centerY, width, height);
                    if (previewDrawn)
                        return true;
                    if (!hoveredXmlTexture.Exists())
                        return false;
                    hoveredXmlTexture.Draw(0, Vector2(centerX, centerY), Vector2(width, height / 2.0f + 0.005f), 0.0f, RGBA::AllWhite());
                    return true;
                },
                0.1f,
                0.0889f
            }
        );
        rowsWithImage.insert(rowsWithImage.end(), statsRows.begin(), statsRows.end());
        StatsPanel::DrawAtOption(optionTextY, 0.100f, rowsWithImage);
        return;
    }

    StatsPanel::DrawAtOption(optionTextY, 0.100f, statsRows);
}

void Sub_SaveFiles()
{
    std::string& _name = dict;
    std::string& _dir = dict3;

    GTAentity myPed = PLAYER_PED_ID();
    const Vector3& myPos = myPed.GetPosition();
    auto& fSaveRangeRadius = _fSaveRangeRadius;

    AddTitle(Language::Translate("menu.spooner.files.title"));
    AddSaveFileSortOption();

    bool bSaveDb = false;
    AddOption(Language::Format("menu.spooner.files.option.save_database.label", {{"count", Databases::EntityDb.size() + Databases::MarkerDb.size() + Databases::LightDb.size()}}), bSaveDb);
    AddOptionDescription(Language::Translate("menu.spooner.files.option.save_database.description"));
    if (bSaveDb)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("menu.spooner.files.option.file_name_prompt.prompt").c_str());
        if (inputStr.length() > 0)
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
            }
            else if (FileManagement::SaveDbToFile(_dir + "\\" + inputStr + ".xml", true))
            {
                Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.saved.message"));
            }
            else
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.save_failed.message"));
                addlog(ige::LogType::LOG_ERROR, "Attempt to save Database file " + inputStr + ".xml failed");
            }
        }
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SpoonerSaveDbToFile, std::string(), 28U, "Enter file name:");
        //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&_dir);
    }

    bool bSaveWorld = false;
    AddOption(Language::Format("menu.spooner.files.option.save_world.label", {{"count", worldEntities.size()}}), bSaveWorld);
    AddOptionDescription(Language::Translate("menu.spooner.files.option.save_world.description"));
    if (bSaveWorld)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("menu.spooner.files.option.file_name_prompt.prompt").c_str());
        if (inputStr.length() > 0)
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
            }
            else if (FileManagement::SaveWorldToFile(_dir + "\\" + inputStr + ".xml", worldEntities, Databases::MarkerDb))
            {
                Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.saved.message"));
            }
            else
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.save_failed.message"));
                addlog(ige::LogType::LOG_ERROR, "Attempt to save World file " + inputStr + ".xml failed");
            }
        }
    }

    std::vector<Entity> vSaveRangeEntities;
    GTAmemory::GetEntityHandles(vSaveRangeEntities, myPos, fSaveRangeRadius);
    bool bSaveRange_plus = false, bSaveRange_minus = false, bSaveRange_save = false;
    AddNumber(Language::Format("menu.spooner.files.option.save_range.label", {{"count", vSaveRangeEntities.size()}}), fSaveRangeRadius, 0, bSaveRange_save, bSaveRange_plus, bSaveRange_minus);
    AddOptionDescription(Language::Translate("menu.spooner.files.option.save_range.description"));
    if (Menu::IsLastDrawnOptionSelected())
        EntityManagement::DrawRadiusDisplayingMarker(myPos, fSaveRangeRadius);
    if (bSaveRange_plus)
    {
        if (fSaveRangeRadius < FLT_MAX)
            fSaveRangeRadius += 1.0f;
    }
    if (bSaveRange_minus)
    {
        if (fSaveRangeRadius > 0.0f)
            fSaveRangeRadius -= 1.0f;
    }
    if (bSaveRange_save)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("menu.spooner.files.option.file_name_prompt.prompt").c_str());
        if (inputStr.length() > 0)
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
            }
            else
            {
                std::vector<SpoonerMarker> vSaveRangeMarkers;
                MarkerManagement::GetAllMarkersInRange(vSaveRangeMarkers, myPos, fSaveRangeRadius);

                if (FileManagement::SaveWorldToFile(_dir + "\\" + inputStr + ".xml", vSaveRangeEntities, vSaveRangeMarkers))
                {
                    Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.saved.message"));
                }
                else
                {
                    Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.save_failed.message"));
                    addlog(ige::LogType::LOG_ERROR, "Attempt to save Range Markers file " + inputStr + ".xml failed");
                }
            }
        }
    }
    AddOption(Language::Translate("menu.spooner.files.option.auto_save.label"), null, nullFunc, SUB::SPOONER_AUTOSAVE);
    /*bool bLoadFromFile = false;
			AddOption(Language::Translate("menu.spooner.files.option.load_from_file.label"), bLoadFromFile); if (bLoadFromFile)
			{
			std::string inputStr = Game::InputBox("", 28U, "Enter file name:");
			if (inputStr.length() > 0)
			{
			if (FileManagement::Exists(_dir + "\\" + inputStr + ".xml"))
			{
			_name = inputStr;
			Menu::pendingSubmenu = SUB::SPOONER_SAVEFILES_LOAD;
			}
			else
			{
			Game::Print::ShowNotification("~r~Error:", "Unable to load file.");
			}
			}
			//OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SpoonerLoadFromFile, std::string(), 28U, "Enter file name:");
			//OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&_name);
			//OnscreenKeyboard::State::arg2._ptr = reinterpret_cast<void*>(&_dir);
			}*/

    std::vector<std::string> vfilnames;
    //get_all_filenames_with_extension(GetPathffA(Pathff::Spooner, false), ".xml", vfilnames, false);
    if (_dir.empty())
        _dir = GetPathffA(Pathff::Spooner, false);
    DIR* dir_point = opendir(_dir.c_str());
    dirent* entry = readdir(dir_point);
    while (entry)
    {
        vfilnames.push_back(entry->d_name);
        entry = readdir(dir_point);
    }
    closedir(dir_point);
    SortSaveFileEntries(vfilnames, _dir);

    bool bCreateFolderPressed = false;
    AddOption(Language::Translate("menu.spooner.files.option.create_folder.label"), bCreateFolderPressed);
    if (bCreateFolderPressed)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("menu.spooner.files.option.folder_prompt.prompt").c_str());
        if (inputStr.length() > 0)
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
            }
            else if (CreateDirectoryA((_dir + "\\" + inputStr).c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
            {
                _dir = _dir + "\\" + inputStr;
                Menu::selectedOptionIndex = 7;
                Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.folder_created.message"));
            }
            else
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.failed.title"), Language::Translate("message.player.wardrobe.create_folder_failed.message"));
                addlog(ige::LogType::LOG_ERROR, "Attempt to create new folder " + inputStr + " failed");
            }
        }
        else
            Game::Print::PrintErrorInvalidInput(inputStr);
        return;
        // No OnscreenKeyboard!
    }

    AddBreak(Language::Translate("menu.spooner.files.section.found_files.label"));

    bool bFolderBackPressed = false;
    AddOption("..", bFolderBackPressed);
    if (bFolderBackPressed)
    {
        std::string baseDir = GetPathffA(Pathff::Spooner, false);
        if (_dir.length() > baseDir.length() && _dir.find(baseDir) == 0)
        {
            _dir = _dir.substr(0, _dir.rfind("\\"));
            Menu::selectedOptionIndex = 7;
        }
        else
        {
            _dir = baseDir;
        }
    }

    if (!vfilnames.empty())
    {
        bool bSearchPressed = false;
        AddOption(_searchStr.empty() ? Language::Translate("menu.spooner.files.option.search.label") : _searchStr, bSearchPressed, nullFunc, -1, true);
        if (bSearchPressed)
        {
            _searchStr = Game::InputBox(_searchStr, 126U, Language::Translate("common.search").c_str(), boost::to_lower_copy(_searchStr));
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
            bool isSp00n = filname.length() > 6 && filname.rfind(".SP00N") == filname.length() - 6;
            TICKOL icon = TICKOL::NONE;
            if (isFolder)
                icon = TICKOL::ARROWRIGHT;
            else if (isXml || isSp00n)
                icon = TICKOL::TICK2;
            bool bFilePressed = false;

            if (isFolder)
            {
                AddTickol(filname + " >>>", true, bFilePressed, bFilePressed, icon, TICKOL::NONE);
                if (bFilePressed)
                {
                    _dir = _dir + "\\" + filname;
                    Menu::selectedOptionIndex = 7;
                }

                if (Menu::IsLastDrawnOptionSelected() && !bFilePressed)
                {
                    if (FolderPreviewBmps_catind::bFolderBmpsEnabled)
                        FolderPreviewBmps_catind::DrawBmp(_dir + "\\" + filname);
                    StatsPanel::DrawFolderContents(_dir + "\\" + filname, currentOptionY + menuPos.y);
                }
            }
            else if (isXml || isSp00n)
            {
                AddTickol(filname, true, bFilePressed, bFilePressed, icon, TICKOL::NONE);
                if (bFilePressed)
                {
                    if (isXml)
                    {
                        _name = filname.substr(0, filname.rfind('.'));
                        Menu::pendingSubmenu = SUB::SPOONER_SAVEFILES_LOAD;
                    }
                    else if (isSp00n)
                    {
                        _name = filname.substr(0, filname.rfind('.'));
                        Menu::pendingSubmenu = SUB::SPOONER_SAVEFILES_LOAD_LEGACYSP00N;
                    }
                }
                if (isXml && Menu::IsLastDrawnOptionSelected() && !bFilePressed)
                {
                    DrawSaveFileStats(_dir + "\\" + filname, currentOptionY + menuPos.y);
                }
            }
        }
    }

    /*vfilnames.clear();
			get_all_filenames_with_extension(GetPathffA(Pathff::Spooner, false), ".SP00N", vfilnames, false);
			if (!vfilnames.empty())
			{
			AddBreak(Language::Translate("menu.spooner.files.section.legacy_sp00n_files.label"));

			for (auto& filname : vfilnames)
			{
			bool bSearchPressed = false;
			AddOption(_searchStr.empty() ? Language::Translate("menu.spooner.files.option.search.label") : _searchStr, bSearchPressed, nullFunc, -1, true); if (bSearchPressed)
			{
			_searchStr = Game::InputBox(_searchStr, 126U, Language::Translate("menu.spooner.files.option.search.prompt").c_str(), boost::to_lower_copy(_searchStr));
			boost::to_upper(_searchStr);
			//OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SearchToUpper, _searchStr, 126U, std::string(), _searchStr);
			//OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&_searchStr);
			}

			if (!_searchStr.empty()) { if (boost::to_upper_copy(filname).find(_searchStr) == std::string::npos) continue; }

			bool bFilePressed = false;
			AddOption(filname, bFilePressed); if (bFilePressed)
			{
			if (FileManagement::Exists(filname, ".SP00N"))
			{
			_name = filname;
			Menu::pendingSubmenu = SUB::SPOONER_SAVEFILES_LOAD_LEGACYSP00N;
			}
			}
			}
			}*/
}

void Sub_AutoSave()
{
    static const std::vector<std::string> intervalNames = {"1 min", "3 min", "5 min", "10 min"};
    static const DWORD intervalValues[] = {60000, 180000, 300000, 600000};
    static const std::vector<std::string> maxFileNames = {"5", "10", "25", "50"};
    static const int maxFileValues[] = {5, 10, 25, 50};

    AddTitle(Language::Translate("menu.spooner.auto_save.title"));

    AddToggle(Language::Translate("menu.spooner.auto_save.option.enable.label"), Settings::bAutoSaveDb);

    int intervalIdx = 0;
    for (int i = 0; i < 4; i++)
    {
        if (Settings::autoSaveIntervalMs == intervalValues[i])
        {
            intervalIdx = i;
            break;
        }
    }
    intervalIdx = AddTexterCycler(Language::Translate("menu.spooner.auto_save.option.interval.label"), intervalIdx, intervalNames);
    AddOptionDescription(Language::Translate("menu.spooner.auto_save.option.interval.description"));
    if (intervalIdx >= 0 && intervalIdx < 4)
        Settings::autoSaveIntervalMs = intervalValues[intervalIdx];

    int maxIdx = 0;
    for (int i = 0; i < 4; i++)
    {
        if (Settings::autoSaveMaxFiles == maxFileValues[i])
        {
            maxIdx = i;
            break;
        }
    }
    maxIdx = AddTexterCycler(Language::Translate("menu.spooner.auto_save.option.max_files.label"), maxIdx, maxFileNames);
    AddOptionDescription(Language::Translate("menu.spooner.auto_save.option.max_files.description"));
    if (maxIdx >= 0 && maxIdx < 4)
        Settings::autoSaveMaxFiles = maxFileValues[maxIdx];
}

void Sub_SaveFiles_Load()
{
    std::string& _name = dict;
    std::string& _dir = dict3;
    std::string filePath = _dir + "\\" + _name + ".xml";

    AddTitle(_name);

    bool bTeleToRef = false;
    AddOption(Language::Translate("menu.spooner.auto_save.option.teleport_reference.label"), bTeleToRef);
    AddOptionDescription(Language::Translate("menu.spooner.auto_save.option.teleport_reference.description"));
    if (bTeleToRef)
    {
        FileManagement::TeleportToReference(filePath);
    }

    static std::string lastLoadedImg = "";
    static DxHookIMG::DxTexture mapPreviewTexture;
    std::string imgPath = _dir + "\\" + _name + ".jpg";
    if (lastLoadedImg != imgPath)
    {
        lastLoadedImg = imgPath;
        mapPreviewTexture = DxHookIMG::DxTexture(); // reset
        std::ifstream f(imgPath);
        if (f.good())
        {
            mapPreviewTexture.Load(imgPath);
        }
    }
    if (mapPreviewTexture.Exists())
    {
        Vector2 res = {0.1f, 0.0889f};
        FLOAT x_coord = 0.324f + menuPos.x;
        FLOAT y_coord = currentOptionY + 0.044f + menuPos.y;
        if (menuPos.x > 0.45f)
            x_coord = menuPos.x - 0.003f;
        DRAW_RECT(x_coord, y_coord, res.x + 0.003f, res.y + 0.003f, 0, 0, 0, 212, false);
        mapPreviewTexture.Draw(0, Vector2(x_coord, y_coord), Vector2(res.x, res.y / 2 + 0.005f), 0.0f, RGBA::AllWhite());
    }

    bool bLoadPlacements = false;
    AddOption(Language::Translate("menu.spooner.auto_save.option.load_placements.label"), bLoadPlacements);
    AddOptionDescription(Language::Translate("menu.spooner.auto_save.option.load_placements.description"));
    if (bLoadPlacements)
    {
        if (FileManagement::LoadPlacementsFromFile(filePath))
        {
            Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.loaded.message"));
        }
        else
        {
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.load_failed.message"));
            addlog(ige::LogType::LOG_ERROR, "Attempt to load placements from " + filePath + ".xml failed");
        }
    }

    bool bRenameFile = false;
    AddOption(Language::Translate("menu.spooner.files.option.rename.label"), bRenameFile);
    if (bRenameFile)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("menu.spooner.files.option.new_name_prompt.prompt").c_str(), _name);
        if (inputStr.length() > 0)
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
            }
            else if (rename(filePath.c_str(), (_dir + "\\" + inputStr + ".xml").c_str()) == 0)
            {
                _name = inputStr;
                Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.renamed.message"));
            }
            else
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.rename_failed.message"));
                addlog(ige::LogType::LOG_ERROR, "Attempt to rename file " + _name + ".xml to " + inputStr + " failed");
            }
        }
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::RenameSpoonerFile, std::string(), 28U, "Enter new name:", fileName);
        //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&_name);
        //OnscreenKeyboard::State::arg2._ptr = reinterpret_cast<void*>(&_dir);
    }

    bool vOverwriteFile = false;
    AddOption(Language::Translate("menu.spooner.files.option.overwrite.label"), vOverwriteFile);
    AddOptionDescription(Language::Translate("menu.spooner.files.option.overwrite.description"));
    if (vOverwriteFile)
    {
        if (FileManagement::SaveDbToFile(filePath, false))
        {
            Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.overwritten.message"));
        }
        else
        {
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.overwrite_failed.message"));
            addlog(ige::LogType::LOG_ERROR, "Attempt to overwrite " + filePath + " failed");
        }
    }

    bool bDeleteFile = false;
    AddOption(Language::Translate("menu.spooner.files.option.delete.label"), bDeleteFile);
    if (bDeleteFile)
    {
        if (remove(filePath.c_str()) == 0)
        {
            Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.deleted.message"));
        }
        else
        {
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.delete_failed.message"));
            addlog(ige::LogType::LOG_ERROR, "Attempt to delete file " + filePath + " failed");
        }
        Menu::SetPreviousMenu();
        Menu::Up();
        return;
    }

    //=============================================================================
    pugi::xml_document doc;
    if (doc.load_file((const char*)filePath.c_str()).status == pugi::status_ok)
    {
        AddBreak(Language::Translate("menu.spooner.files.section.attributes.label"));
        auto nodeRoot = doc.child("SpoonerPlacements"); // Root

        auto nodeNote = nodeRoot.child("Note");
        if (nodeNote)
        {
            std::string noteStr = nodeNote.text().as_string();
            bool bEditNotePressed = false;
            AddTexter(
                Language::Translate("menu.spooner.files.option.note.label"),
                0,
                std::vector<std::string>{noteStr.length() > 0 ? (noteStr.length() < 10 ? noteStr : noteStr.substr(0, 10) + "...") : Language::Translate("menu.spooner.option.files.value.none")},
                bEditNotePressed
            );
            AddOptionDescription(Language::Translate("menu.spooner.files.option.note.description"));
            if (bEditNotePressed)
            {
                std::string inputStr = Game::InputBox("~`", 300U, Language::Translate("menu.spooner.files.option.note_prompt.prompt").c_str(), noteStr);
                if (inputStr.compare("~`") != 0)
                {
                    noteStr = inputStr;
                    nodeNote.text() = noteStr.c_str();
                    doc.save_file((const char*)filePath.c_str());
                }
                //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SpoonerFileNote, "~`", 300U, "Enter note:", noteStr);
                //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&fileName);
            }
        }

        auto nodeAudioFile = nodeRoot.child("AudioFile");
        if (nodeAudioFile)
        {
            std::string audioFileName = nodeAudioFile.text().as_string();
            bool bEditAudioFileNamePressed = false;
            AddTexter(
                Language::Translate("menu.spooner.files.option.audio.label"),
                0,
                std::vector<std::string>{audioFileName.length() > 0 ? audioFileName : Language::Translate("menu.spooner.option.files.value.none")},
                bEditAudioFileNamePressed
            );
            AddOptionDescription(Language::Translate("menu.spooner.files.option.audio.description"));
            if (bEditAudioFileNamePressed)
            {
                std::string inputStr = Game::InputBox("~`", 64U, Language::Translate("menu.spooner.files.option.audio_prompt.prompt").c_str(), audioFileName);
                if (inputStr.compare("~`") != 0)
                {
                    audioFileName = inputStr;
                    nodeAudioFile.text() = audioFileName.c_str();
                    doc.save_file((const char*)filePath.c_str());
                }
                //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SpoonerFileAudioFile, "~`", 64U, "Enter filename with extension (file should be in menyooStuff\\Audio):", audioFileName);
                //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&fileName);
            }
        }

        auto nodeStartTaskSeqOnLoad = nodeRoot.child("StartTaskSequencesOnLoad");
        if (nodeStartTaskSeqOnLoad)
        {
            bool bToggleStartTaskSeqOnLoadPressed = false;
            AddTickol(
                Language::Translate("menu.spooner.files.option.start_task_sequences.label"),
                nodeStartTaskSeqOnLoad.text().as_bool(),
                bToggleStartTaskSeqOnLoadPressed,
                bToggleStartTaskSeqOnLoadPressed,
                TICKOL::BOXTICK,
                TICKOL::BOXBLANK
            );
            AddOptionDescription(Language::Translate("menu.spooner.files.option.start_task_sequences.description"));
            if (bToggleStartTaskSeqOnLoadPressed)
            {
                nodeStartTaskSeqOnLoad.text() = !nodeStartTaskSeqOnLoad.text().as_bool();
                doc.save_file((const char*)filePath.c_str());
            }
        }

        auto nodeClearDatabase = nodeRoot.child("ClearDatabase");
        if (nodeClearDatabase)
        {
            bool bToggleClearDatabasePressed = false;
            AddTickol(
                Language::Translate("menu.spooner.files.option.clear_database.label"),
                nodeClearDatabase.text().as_bool(),
                bToggleClearDatabasePressed,
                bToggleClearDatabasePressed,
                TICKOL::BOXTICK,
                TICKOL::BOXBLANK
            );
            AddOptionDescription(Language::Translate("menu.spooner.files.option.clear_database.description"));
            if (bToggleClearDatabasePressed)
            {
                nodeClearDatabase.text() = !nodeClearDatabase.text().as_bool();
                doc.save_file((const char*)filePath.c_str());
            }
        }

        auto nodeClearMarkers = nodeRoot.child("ClearMarkers");
        if (nodeClearMarkers)
        {
            bool bToggleClearMarkersPressed = false;
            AddTickol(
                Language::Translate("menu.spooner.files.option.clear_markers.label"),
                nodeClearMarkers.text().as_bool(),
                bToggleClearMarkersPressed,
                bToggleClearMarkersPressed,
                TICKOL::BOXTICK,
                TICKOL::BOXBLANK
            );
            AddOptionDescription(Language::Translate("menu.spooner.files.option.clear_markers.description"));
            if (bToggleClearMarkersPressed)
            {
                nodeClearMarkers.text() = !nodeClearMarkers.text().as_bool();
                doc.save_file((const char*)filePath.c_str());
            }
        }

        Vector3 refCoords, imgLoadingCoords;
        auto nodeReferenceCoords = nodeRoot.child("ReferenceCoords");
        auto nodeImgLoadingCoords = nodeRoot.child("ImgLoadingCoords");
        for (auto& nas : std::vector<std::pair<Vector3*, pugi::xml_node*>>{{&refCoords, &nodeReferenceCoords}, {&imgLoadingCoords, &nodeImgLoadingCoords}})
        {
            if (*nas.second)
            {
                nas.first->x = nas.second->child("X").text().as_float();
                nas.first->y = nas.second->child("Y").text().as_float();
                nas.first->z = nas.second->child("Z").text().as_float();

                EntityManagement::DrawRadiusDisplayingMarker(*nas.first, 0.5f, RGBA(0, 102, 204, 130));
                Vector2 scrnPos;
                if (World::WorldToScreen(*nas.first, scrnPos))
                {
                    Game::Print::SetupDraw(GTAfont::Impact, Vector2(0.3f, 0.3f), true, false, true);
                    Game::Print::drawstring(nas.second->name(), scrnPos.x, scrnPos.y);
                }
            }
        }

        auto nodeClearWorld = nodeRoot.child("ClearWorld");
        if (nodeClearDatabase && nodeReferenceCoords)
        {
            float clearWorldRadius = nodeClearWorld.text().as_float();
            bool clearWorld_plus = false, clearWorld_minus = false;
            AddNumber(Language::Translate("menu.spooner.files.option.clear_world.label"), clearWorldRadius, 0, null, clearWorld_plus, clearWorld_minus);
            AddOptionDescription(Language::Translate("menu.spooner.files.option.clear_world.description"));
            if (Menu::IsLastDrawnOptionSelected())
                EntityManagement::DrawRadiusDisplayingMarker(refCoords, clearWorldRadius, RGBA(255, 0, 0, 130));
            if (clearWorld_plus)
            {
                if (clearWorldRadius < FLT_MAX)
                {
                    clearWorldRadius += 1.0f;
                    nodeClearWorld.text() = abs(clearWorldRadius);
                    doc.save_file((const char*)filePath.c_str());
                }
            }
            if (clearWorld_minus)
            {
                if (clearWorldRadius > 0.0f)
                {
                    clearWorldRadius -= 1.0f;
                    nodeClearWorld.text() = abs(clearWorldRadius);
                    doc.save_file((const char*)filePath.c_str());
                }
            }
        }

        for (auto& nas : std::vector<std::tuple<std::string, Vector3*, pugi::xml_node*, std::string>>{
                 std::make_tuple("Reference Coordinates", &refCoords, &nodeReferenceCoords, "ReferenceCoords"),
                 std::make_tuple("CD-Image Loading Coordinates", &imgLoadingCoords, &nodeImgLoadingCoords, "ImgLoadingCoords")
             })
        {
            auto& xNode = *std::get<2>(nas);
            AddBreak(
                Language::Translate(std::get<3>(nas) == "ReferenceCoords" ? "menu.spooner.files.section.reference_coordinates.label" : "menu.spooner.files.section.image_loading_coordinates.label")
            );
            AddOption("~italic~" + (xNode ? std::get<1>(nas)->ToString() : Language::Translate("menu.spooner.files.option.not_set.label")), null);

            auto& spoocam = SpoonerCamera::camera;
            if (!spoocam.IsActive())
            {
                bool bSetPosToMe = false;
                AddOption(Language::Translate("menu.spooner.files.option.set_to_player_position.label"), bSetPosToMe);
                if (bSetPosToMe)
                {
                    Vector3 myPos = GTAentity(PLAYER_PED_ID()).GetPosition();
                    *std::get<1>(nas) = myPos;
                    if (!xNode)
                    {
                        xNode = nodeRoot.append_child(std::get<3>(nas).c_str());
                        xNode.append_child("X");
                        xNode.append_child("Y");
                        xNode.append_child("Z");
                    }
                    xNode.child("X").text() = std::get<1>(nas)->x;
                    xNode.child("Y").text() = std::get<1>(nas)->y;
                    xNode.child("Z").text() = std::get<1>(nas)->z;
                    doc.save_file((const char*)filePath.c_str());
                }
            }
            else
            {
                bool bSetPosToHitCoords = false;
                AddOption(Language::Translate("menu.spooner.files.option.set_to_camera_target.label"), bSetPosToHitCoords);
                AddOptionDescription(Language::Translate("menu.spooner.files.option.set_to_camera_target.description"));
                if (bSetPosToHitCoords)
                {
                    Vector3 hitCoords = spoocam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 160.0f, 3.0f);
                    *std::get<1>(nas) = hitCoords;
                    if (!xNode)
                    {
                        xNode = nodeRoot.append_child(std::get<3>(nas).c_str());
                        xNode.append_child("X");
                        xNode.append_child("Y");
                        xNode.append_child("Z");
                    }
                    xNode.child("X").text() = std::get<1>(nas)->x;
                    xNode.child("Y").text() = std::get<1>(nas)->y;
                    xNode.child("Z").text() = std::get<1>(nas)->z;
                    doc.save_file((const char*)filePath.c_str());
                }
            }
            if (IS_WAYPOINT_ACTIVE())
            {
                bool bSetPosToWp = false;
                AddOption(Language::Translate("menu.spooner.files.option.set_to_waypoint.label"), bSetPosToWp);
                if (bSetPosToWp)
                {
                    GTAblip wpBlip = GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint);
                    Vector3 wpCoords = wpBlip.GetPosition();
                    wpCoords.z = World::GetGroundHeight(wpCoords);
                    *std::get<1>(nas) = wpCoords;
                    if (!xNode)
                    {
                        xNode = nodeRoot.append_child(std::get<3>(nas).c_str());
                        xNode.append_child("X");
                        xNode.append_child("Y");
                        xNode.append_child("Z");
                    }
                    xNode.child("X").text() = std::get<1>(nas)->x;
                    xNode.child("Y").text() = std::get<1>(nas)->y;
                    xNode.child("Z").text() = std::get<1>(nas)->z;
                    doc.save_file((const char*)filePath.c_str());
                }
            }
        }

        auto nodeTimecycMod = nodeRoot.child("TimecycleModifier");
        if (nodeTimecycMod)
        {
            AddBreak(Language::Translate("menu.spooner.files.section.vision_hax.label"));

            std::string timecycModStr = nodeTimecycMod.text().as_string();
            bool bTimecycMod_pressed = false, bTimecycMod_plus = false, bTimecycMod_minus = false;
            AddTexter(
                Language::Translate("menu.spooner.files.option.timecycle_mod.label"),
                0,
                std::vector<std::string>{timecycModStr.length() ? timecycModStr : Language::Translate("menu.spooner.option.files.value.none")},
                bTimecycMod_pressed,
                bTimecycMod_plus,
                bTimecycMod_minus
            );
            AddOptionDescription(Language::Translate("menu.spooner.files.option.timecycle_mod.description"));
            if (bTimecycMod_plus)
            {
                auto tit = std::find_if(
                    TimecycleModification::vTimecycles.begin(),
                    TimecycleModification::vTimecycles.end(),
                    [timecycModStr](const std::pair<std::string, std::string>& item) { return item.first.compare(timecycModStr) == 0; }
                );
                if (tit == TimecycleModification::vTimecycles.end())
                {
                    timecycModStr = TimecycleModification::vTimecycles.front().first;
                    nodeTimecycMod.text() = timecycModStr.c_str();
                    doc.save_file((const char*)filePath.c_str());
                }
                else
                {
                    ++tit;
                    if (tit != TimecycleModification::vTimecycles.end())
                    {
                        timecycModStr = tit->first;
                        nodeTimecycMod.text() = timecycModStr.c_str();
                        doc.save_file((const char*)filePath.c_str());
                    }
                }
            }
            else if (bTimecycMod_minus)
            {
                auto tit = std::find_if(
                    TimecycleModification::vTimecycles.begin(),
                    TimecycleModification::vTimecycles.end(),
                    [timecycModStr](const std::pair<std::string, std::string>& item) { return item.first.compare(timecycModStr) == 0; }
                );
                if (tit == TimecycleModification::vTimecycles.begin())
                {
                    timecycModStr.clear();
                    nodeTimecycMod.text() = timecycModStr.c_str();
                    doc.save_file((const char*)filePath.c_str());
                }
                else if (tit != TimecycleModification::vTimecycles.end())
                {
                    --tit;
                    timecycModStr = tit->first;
                    nodeTimecycMod.text() = timecycModStr.c_str();
                    doc.save_file((const char*)filePath.c_str());
                }
            }
            else if (bTimecycMod_pressed)
            {
                std::string inputStr = Game::InputBox(timecycModStr, 28U, Language::Translate("menu.spooner.files.option.timecycle_mod_prompt.prompt").c_str(), timecycModStr);
                if (inputStr.compare(timecycModStr) != 0)
                {
                    timecycModStr = inputStr;
                    nodeTimecycMod.text() = timecycModStr.c_str();
                    doc.save_file((const char*)filePath.c_str());
                }
                // No OnScreenKeyboard!
            }

            float timecycModStrength = nodeTimecycMod.attribute("strength").as_float(1.0f);
            bool bTimecycModStrength_plus = false, bTimecycModStrength_minus = false;
            AddNumber(Language::Translate("menu.spooner.files.option.strength.label"), timecycModStrength, 2, null, bTimecycModStrength_plus, bTimecycModStrength_minus);
            AddOptionDescription(Language::Translate("menu.spooner.files.option.strength.description"));
            if (bTimecycModStrength_plus)
            {
                if (timecycModStrength < 3.0f)
                {
                    timecycModStrength += 0.02f;
                    nodeTimecycMod.attribute("strength") = timecycModStrength;
                    doc.save_file((const char*)filePath.c_str());
                }
            }
            else if (bTimecycModStrength_minus)
            {
                if (timecycModStrength > 0.0f)
                {
                    currentTimecycleStrength -= 0.02f;
                    nodeTimecycMod.attribute("strength") = timecycModStrength;
                    doc.save_file((const char*)filePath.c_str());
                }
            }
        }

        auto nodeWeatherToSet = nodeRoot.child("WeatherToSet");
        if (nodeWeatherToSet)
        {
            AddBreak(Language::Translate("menu.spooner.files.section.weather_to_set.label"));
            std::string weatherToSetStr = nodeWeatherToSet.text().as_string();
            int weatherToSetInt = static_cast<int>(World::GetWeather(weatherToSetStr));
            //bool weatherToSet_plus = false, weatherToSet_minus = false;
            //AddTexter("Weather To Set", (weatherToSetInt == -1 ? 0 : weatherToSetInt), (weatherToSetInt == -1 ? std::vector<std::string>{"None"} : World::sWeatherNames), null, weatherToSet_plus, weatherToSet_minus);
            //if (weatherToSet_plus){ if (weatherToSetInt < World::sWeatherNames.size() - 1){ weatherToSetInt++; nodeWeatherToSet.text() = World::WeatherName_get(static_cast<WeatherType>(weatherToSetInt)).c_str(); doc.save_file((const char*)filePath.c_str()); } }
            //if (weatherToSet_minus){ if (weatherToSetInt > -1){ weatherToSetInt--; nodeWeatherToSet.text() = World::WeatherName_get(static_cast<WeatherType>(weatherToSetInt)).c_str(); doc.save_file((const char*)filePath.c_str()); } }

            bool bWeatherNonePressed = false;
            AddTickol(Language::Translate("menu.spooner.files.option.none.label"), weatherToSetInt == -1, bWeatherNonePressed, bWeatherNonePressed);
            if (bWeatherNonePressed)
            {
                nodeWeatherToSet.text() = "";
                doc.save_file((const char*)filePath.c_str());
            }
            for (auto& ws : World::sWeatherNames)
            {
                bool bWeatherNamePressed = false;
                AddTickol(ws.first, weatherToSetStr == ws.second, bWeatherNamePressed, bWeatherNamePressed);
                if (bWeatherNamePressed)
                {
                    nodeWeatherToSet.text() = ws.second.c_str();
                    doc.save_file((const char*)filePath.c_str());
                }
            }
        }
    }
}

void Sub_SaveFiles_Load_LegacySP00N()
{
    std::string& _name = dict;
    std::string& _dir = dict3;
    std::string filePath = _dir + "\\" + _name + ".SP00N";

    AddTitle(_name);

    bool bLoadPlacements = false;
    AddOption(Language::Translate("menu.spooner.files.option.load_placements.label"), bLoadPlacements);
    AddOptionDescription(Language::Translate("menu.spooner.files.option.load_placements.description"));
    if (bLoadPlacements)
    {
        if (FileManagement::LoadPlacementsFromSP00NFile(filePath))
        {
            Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.loaded.message"));
        }
        else
        {
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.load_failed.message"));
            addlog(ige::LogType::LOG_ERROR, "Attempt to load Placements file from" + filePath + " failed");
        }
    }

    bool bRenameFile = false;
    AddOption(Language::Translate("menu.spooner.files.option.rename.label"), bRenameFile);
    if (bRenameFile)
    {
        std::string inputStr = Game::InputBox("", 28U, Language::Translate("menu.spooner.files.option.new_name_prompt.prompt").c_str(), _name);
        if (inputStr.length() > 0)
        {
            if (!IsSafePath(inputStr))
            {
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
            }
            else if (rename(filePath.c_str(), (_dir + "\\" + inputStr + ".SP00N").c_str()) == 0)
            {
                _name = inputStr;
                Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.renamed.message"));
            }
            else
            {
                addlog(ige::LogType::LOG_ERROR, "Attempt to rename file from" + _name + " to " + inputStr + ".SP00N failed");
            }
        }
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::RenameSp00nFile, std::string(), 28U, "Enter new name:", fileName);
        //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&_name);
        //OnscreenKeyboard::State::arg2._ptr = reinterpret_cast<void*>(&_dir);
    }

    bool bDeleteFile = false;
    AddOption(Language::Translate("menu.spooner.files.option.delete.label"), bDeleteFile);
    if (bDeleteFile)
    {
        if (remove(filePath.c_str()) == 0)
        {
            Game::Print::PrintBottomLeft(Language::Translate("message.spooner.files.deleted.message"));
        }
        else
        {
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.delete_failed.message"));
            addlog(ige::LogType::LOG_ERROR, "Attempt to delete file" + filePath + " failed");
        }
        Menu::SetPreviousMenu();
        Menu::Up();
        return;
    }
}

void Sub_ManageEntities()
{
    selectedEntity.handle = 0;

    SpoonerEntity* p_entityToDelete = nullptr;
    GTAentity myPed = PLAYER_PED_ID();
    auto myIndexInDb = EntityManagement::GetEntityIndexInDb(myPed);
    bool bIsSelfInDb = myIndexInDb >= 0;

    AddTitle(Language::Translate("menu.spooner.manage.title"));

    AddOption(Language::Translate("menu.spooner.manage.option.removal.label"), null, nullFunc, SUB::SPOONER_MANAGEDB_REMOVAL);
    AddOptionDescription(Language::Translate("menu.spooner.manage.option.removal.description"));

    bool bPressedSelf = false;
    AddOption(
        Language::Format(
            "menu.spooner.manage.option.self.label",
            {{"state", Language::Translate(bIsSelfInDb ? "menu.spooner.manage.self.option.in_database.label" : "menu.spooner.manage.self.option.not_in_database.label")}}
        ),
        bPressedSelf
    );
    AddOptionDescription(Language::Translate("menu.spooner.manage.option.self.description"));
    if (bPressedSelf)
    {
        if (bIsSelfInDb)
            selectedEntity = Databases::EntityDb[myIndexInDb];
        else
        {
            selectedEntity = SpoonerEntity();
            selectedEntity.dynamic = true;
            selectedEntity.handle = myPed;
            selectedEntity.hashName = GetPedModelLabel(myPed.Model(), true);
            if (selectedEntity.hashName.length() == 0)
                selectedEntity.hashName = "Player (Unk Model)";
            selectedEntity.isStill = true;
            selectedEntity.type = EntityType::PED;
        }
        Menu::pendingSubmenu = SUB::SPOONER_SELECTEDENTITYOPS;
        return;
    }

    if (!Databases::EntityDb.empty())
    {
        AddBreak(Language::Translate("menu.spooner.manage.section.database.label"));

        bool bEntTypeToShow_plus = false, bEntTypeToShow_minus = false;
        AddTexter(
            Language::Translate("menu.spooner.manage.option.show.label"),
            _entTypeToShowTexterValue,
            std::vector<std::string>{
                Language::Translate("menu.spooner.option.manage.value.all"),
                Language::Translate("menu.spooner.option.manage.value.peds"),
                Language::Translate("menu.spooner.option.manage.value.vehicles"),
                Language::Translate("menu.spooner.option.manage.value.objects")
            },
            null,
            bEntTypeToShow_plus,
            bEntTypeToShow_minus
        );
        if (bEntTypeToShow_plus)
        {
            if (_entTypeToShowTexterValue < 3)
                _entTypeToShowTexterValue++;
        }
        if (bEntTypeToShow_minus)
        {
            if (_entTypeToShowTexterValue > 0)
                _entTypeToShowTexterValue--;
        }

        for (UINT i = 0; i < Databases::EntityDb.size(); i++)
        {
            auto& e = Databases::EntityDb[i];
            if (_entTypeToShowTexterValue != 0 && static_cast<UINT8>(e.type) != _entTypeToShowTexterValue)
                continue;
            bool bEntityExists = e.handle.Exists();
            bool bEntityPressed = false;

            const std::string& strEntTypeConcat = (*Menu::activeOptionIndex == Menu::currentOptionCount + 1 ? "  ~bold~[" + e.TypeName() + "]~bold~" : std::string());
            AddOption(e.hashName + (bEntityExists ? "" : " (Invalid)") + strEntTypeConcat, bEntityPressed);
            if (bEntityPressed)
            {
                if (bEntityExists)
                {
                    selectedEntity = e;
                    Menu::pendingSubmenu = SUB::SPOONER_SELECTEDENTITYOPS;
                }
            }


            if (Menu::IsLastDrawnOptionSelected())
            {
                EntityManagement::ShowArrowAboveEntity(e.handle);

                bool bShortcutDeletePressed;
                Keybinds::AddBindIB(
                    "menu_action",
                    bEntityExists,
                    Language::Translate("menu.spooner.manage.option.delete_entity.label").c_str(),
                    Language::Translate("menu.spooner.manage.option.remove_invalid.label").c_str()
                );
                bShortcutDeletePressed = Keybinds::WasPressedThisFrame("menu_action");

                if (bShortcutDeletePressed)
                {
                    p_entityToDelete = &e;
                    //i--;
                    //if (*Menu::activeOptionIndex >= Menu::totalOptionCount) Menu::Up();
                }
            }
        }
    }

    if (p_entityToDelete != nullptr)
    {
        auto e = *p_entityToDelete;
        if (e.handle.Exists())
        {
            e.handle.RequestControl(600);
            EntityManagement::DeleteEntity(e);
        }
        else
        {
            EntityManagement::RemoveEntityFromDb(e);
        }
    }

    if (Menu::IsSelectionPastDrawnOptions())
        Menu::Up(); // Go up if you're too far down due to removing an entity from DB from the properties menu and coming back here again.
}

void Sub_ManageEntities_Removal()
{
    AddTitle(Language::Translate("menu.spooner.manage.removal.title"));

    //AddOption("Select DB Entities To Delete", null, nullFunc, SUB::SPOONER_MANAGEDB_REMOVAL_FROMDB);

    AddBreak(Language::Translate("menu.spooner.manage.section.database.label"));
    AddOption(Language::Format("menu.spooner.manage.option.delete_markers.label", {{"count", Databases::MarkerDb.size()}}), null, MarkerManagement::RemoveAllMarkers);
    AddOption(Language::Format("menu.spooner.manage.option.delete_lights.label", {{"count", Databases::LightDb.size()}}), null, LightManagement::RemoveAll);
    AddOption(Language::Format("menu.spooner.manage.option.delete_entities_db.label", {{"count", Databases::EntityDb.size()}}), null, EntityManagement::DeleteAllEntitiesInDb);
    AddOption(Language::Translate("menu.spooner.manage.option.delete_objects_db.label"), null, EntityManagement::DeleteAllPropsInDb);
    AddOption(Language::Translate("menu.spooner.manage.option.delete_peds_db.label"), null, EntityManagement::DeleteAllPedsInDb);
    AddOption(Language::Translate("menu.spooner.manage.option.delete_vehicles_db.label"), null, EntityManagement::DeleteAllVehiclesInDb);
    AddOption(Language::Translate("menu.spooner.manage.option.delete_invalid_db.label"), null, EntityManagement::DeleteInvalidEntitiesInDb);
    AddOptionDescription(Language::Translate("menu.spooner.manage.option.delete_invalid_db.description"));

    AddBreak(Language::Translate("menu.spooner.manage.section.world.label"));
    AddOption(Language::Format("menu.spooner.manage.option.delete_entities_world.label", {{"count", worldEntities.size()}}), null, EntityManagement::DeleteAllEntitiesInWorld);
    AddOption(Language::Format("menu.spooner.manage.option.delete_objects_world.label", {{"count", worldObjects.size()}}), null, EntityManagement::DeleteAllPropsInWorld);
    AddOption(Language::Format("menu.spooner.manage.option.delete_peds_world.label", {{"count", worldPeds.size()}}), null, EntityManagement::DeleteAllPedsInWorld);
    AddOption(Language::Format("menu.spooner.manage.option.delete_vehicles_world.label", {{"count", worldVehicles.size()}}), null, EntityManagement::DeleteAllVehiclesInWorld);

    AddOption(Language::Translate("menu.spooner.manage.option.clear_db.label"), null, EntityManagement::ClearDb);
    AddOptionDescription(Language::Translate("menu.spooner.manage.option.clear_db.description"));
}

/*void Sub_ManageEntities_Removal_FromDb()
		{
			if (Databases::EntityDb.empty())
			{
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.entity.database_empty.message"));
				Menu::SetPreviousMenu();
				return;
			}

			AddTitle(Language::Translate("menu.spooner.manage.delete_entities.title"));

			for (auto& e : Databases::EntityDb)
			{
				bool bEntityExists = e.handle.Exists();
				bool bEntityPressed = false;
				AddOption(e.hashName + (bEntityExists ? "" : " (Invalid)"), bEntityPressed);
					if (Menu::IsLastDrawnOptionSelected()) EntityManagement::ShowArrowAboveEntity(e.handle, RGBA(255, 0, 0, 200));
				if (bEntityPressed)
				{
					if (bEntityExists)
					{
						e.handle.RequestControl(600);
						EntityManagement::DeleteEntity(e);
					}
					else
					{
						EntityManagement::RemoveEntityFromDb(e);
					}
					break;
				}
			}

		}*/
void Sub_SelectedEntityOps()
{
    SpoonerMode::editingState.SetMode(SpoonerMode::eEditMode::Disabled);
    if (!selectedEntity.handle.Exists())
    {
        Menu::SetPreviousMenu();
        return;
    }
    selectedEntity.handle.RequestControlOnce();
    auto thisEntityIndexInDb = EntityManagement::GetEntityIndexInDb(selectedEntity);
    bool isThisEntityInDb = thisEntityIndexInDb >= 0;
    if (isThisEntityInDb)
    {
        selectedEntity.taskSequence = Databases::EntityDb[thisEntityIndexInDb].taskSequence;
        Databases::EntityDb[thisEntityIndexInDb] = selectedEntity;
    }
    Model selectedEntityModel = selectedEntity.handle.Model();

    AddTitle(selectedEntity.hashName);

    switch (selectedEntity.type)
    {
    case EntityType::PROP:
    {
        bool bIsAFav = FavouritesManagement::IsPropAFavourite(selectedEntity.hashName, selectedEntityModel.hash);
        bool bAddToFav = false, bRemoveFromFav = false;
        AddTickol(Language::Translate("menu.spooner.entity.option.model_favourite.label"), bIsAFav, bAddToFav, bRemoveFromFav, TICKOL::BOXTICK, TICKOL::BOXBLANK);
        if (bAddToFav)
        {
            FavouritesManagement::AddPropToFavourites(selectedEntity.hashName, selectedEntityModel.hash);
        }
        else if (bRemoveFromFav)
        {
            FavouritesManagement::RemovePropFromFavourites(selectedEntity.hashName, selectedEntityModel.hash);
        }
    }
    break;
    case EntityType::VEHICLE:
    {
        bool bIsAFav = FavouritesManagement::IsVehicleAFavourite(selectedEntityModel);
        bool bAddToFav = false, bRemoveFromFav = false;
        AddTickol(Language::Translate("menu.spooner.entity.option.model_favourite.label"), bIsAFav, bAddToFav, bRemoveFromFav, TICKOL::BOXTICK, TICKOL::BOXBLANK);
        if (bAddToFav)
        {
            std::string inputStr = Game::InputBox("", 28U, Language::Translate("menu.spooner.entity.option.custom_name_prompt.prompt").c_str(), selectedEntityModel.VehicleDisplayName(true));
            if (inputStr.length() > 0)
            {
                FavouritesManagement::AddVehicleToFavourites(selectedEntityModel, inputStr);
            }
            //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::FavouriteVehicleModelCurrent, std::string(), 28U, "Enter custom name:", selectedEntityModel.VehicleDisplayName(true));
            //OnscreenKeyboard::State::arg1._uint = selectedEntityModel.hash;
        }
        else if (bRemoveFromFav)
        {
            FavouritesManagement::RemoveVehicleFromFavourites(selectedEntityModel);
        }
    }
    break;
    }

    bool bAddToOrDb = false, bRemoveFromDb = false;
    AddTickol(Language::Translate("menu.spooner.entity.option.in_database.label"), isThisEntityInDb, bAddToOrDb, bRemoveFromDb, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.entity.option.in_database.description"));
    if (bAddToOrDb)
    {
        EntityManagement::AddEntityToDb(selectedEntity);
        Game::Print::PrintBottomLeft(Language::Format("message.spooner.entity.added_to_database.message", {{"entity", selectedEntity.hashName}}));
    }
    else if (bRemoveFromDb)
    {
        EntityManagement::RemoveEntityFromDb(selectedEntity);
        Game::Print::PrintBottomLeft(Language::Format("message.spooner.entity.removed_from_database.message", {{"entity", selectedEntity.hashName}}));
        if (Menu::submenuHistory[Menu::menuHistoryIndex] == SUB::SPOONER_MANAGEDB)
        {
            Menu::optionSelectionHistory[Menu::menuHistoryIndex] -= 1;
        }
    }

    bool bCopyPressed = false, bCopy_plus = false, bCopy_minus = false;
    AddTexter(
        Language::Translate("menu.spooner.entity.option.copy.label"),
        _copyEntTexterValue,
        std::vector<std::string>{Language::Translate("menu.spooner.entity.option.copy.value.entity_only"), Language::Translate("menu.spooner.entity.option.copy.value.with_attachments")},
        bCopyPressed,
        bCopy_plus,
        bCopy_minus
    );
    AddOptionDescription(Language::Translate("menu.spooner.entity.option.copy.description"));
    if (bCopy_plus)
    {
        if (_copyEntTexterValue < 1U)
            _copyEntTexterValue++;
    }
    if (bCopy_minus)
    {
        if (_copyEntTexterValue > 0)
            _copyEntTexterValue--;
    }
    if (bCopyPressed)
    {
        const SpoonerEntity& copiedEntity = EntityManagement::CopyEntity(selectedEntity, isThisEntityInDb, true, _copyEntTexterValue);
        //EntityManagement::AddEntityToDb(copiedEntity);
        selectedEntity = copiedEntity;
    }

    bool bDeletePressed = false;
    AddOption(Language::Translate("menu.spooner.entity.option.delete.label"), bDeletePressed);
    if (bDeletePressed)
    {
        selectedEntity.handle.RequestControl(600);
        EntityManagement::DeleteEntity(selectedEntity);
        if (Menu::submenuHistory[Menu::menuHistoryIndex] == SUB::SPOONER_MANAGEDB)
        {
            Menu::optionSelectionHistory[Menu::menuHistoryIndex] -= 1;
        }
        Menu::SetPreviousMenu();
        return;
    }

    sub::Spooner::SpoonerBlip* existingBlip = nullptr;
    for (auto& blip : Databases::BlipDb)
    {
        if (blip.BlipType == SpoonerBlip::Type::Entity && blip.EntityHandle == selectedEntity.handle.GetHandle())
        {
            existingBlip = &blip;
            break;
        }
    }

    if (existingBlip)
    {
        bool bDeleteBlipPressed = false;
        AddOption(Language::Translate("menu.spooner.entity.option.delete_blip.label"), bDeleteBlipPressed);
        if (bDeleteBlipPressed)
        {
            BlipCustoms::RemoveBlip(*existingBlip);
        }
    }
    else
    {
        bool bAddEntityBlipPressed = false;
        AddOption(Language::Translate("menu.spooner.entity.option.add_blip.label"), bAddEntityBlipPressed);
        if (bAddEntityBlipPressed)
        {
            sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(SpoonerBlip::Type::Entity, selectedEntity.hashName);

            auto mapping = GetBlipMappingForEntity(selectedEntity.handle);
            sub::Spooner::SelectedBlip->EntityHandle = selectedEntity.handle.GetHandle();
            sub::Spooner::SelectedBlip->bAttached = true;
            sub::Spooner::SelectedBlip->Icon = mapping.icon;
            sub::Spooner::SelectedBlip->bSyncRotation = mapping.syncRotation;
            sub::Spooner::SelectedBlip->Alpha = 255;
            sub::Spooner::SelectedBlip->Scale = (mapping.icon == BlipIcon::Standard || mapping.icon == BlipIcon::Enemy) ? 0.80f : 1.0f;
            BlipCustoms::RefreshBlip(*sub::Spooner::SelectedBlip);
            Menu::pendingSubmenu = SUB::SPOONER_BLIPS_ENTITYINBLIP;
        }
    }

    bool bDynamicPressed = false;
    AddLocal(Language::Translate("menu.spooner.entity.option.dynamic.label"), selectedEntity.dynamic, bDynamicPressed, bDynamicPressed);
    AddOptionDescription(Language::Translate("menu.spooner.entity.option.dynamic.description"));
    if (bDynamicPressed)
    {
        selectedEntity.dynamic = !selectedEntity.dynamic;
        selectedEntity.handle.SetDynamic(selectedEntity.dynamic);
        selectedEntity.handle.FreezePosition(!selectedEntity.dynamic);
    }

    bool bFreezePosPressed = false;
    AddLocal(Language::Translate("menu.spooner.entity.option.frozen.label"), selectedEntity.handle.IsPositionFrozen(), bFreezePosPressed, bFreezePosPressed);
    if (bFreezePosPressed)
    {
        selectedEntity.handle.FreezePosition(!selectedEntity.handle.IsPositionFrozen());
    }
    if (selectedEntity.type == EntityType::VEHICLE && selectedEntity.handle.GetLandingGearState() != -1)
    {
        bool bLandingGearDown = (selectedEntity.handle.GetLandingGearState() == 0); // 0 = Deployed
        bool oldValue = bLandingGearDown;

        AddToggle(Language::Translate("menu.spooner.entity.option.landing_gear.label"), bLandingGearDown);

        if (bLandingGearDown != oldValue)
        {
            selectedEntity.handle.SetLandingGear(bLandingGearDown);
        }
    }

    if (selectedEntity.type == EntityType::PROP)
    {
        auto& thisTextureVariation = selectedEntity.textureVariation;
        bool bTextureVariation_plus = false, bTextureVariation_minus = false;
        AddNumber(Language::Translate("menu.spooner.entity.option.texture_variation.label"), thisTextureVariation, 0, null, bTextureVariation_plus, bTextureVariation_minus);
        AddOptionDescription(Language::Translate("menu.spooner.entity.option.texture_variation.description"));
        if (bTextureVariation_plus)
        {
            if (thisTextureVariation < UINT8_MAX)
            {
                thisTextureVariation++;
                SET_OBJECT_TINT_INDEX(selectedEntity.handle.Handle(), thisTextureVariation);
            }
        }
        if (bTextureVariation_minus)
        {
            if (thisTextureVariation > 0)
            {
                thisTextureVariation--;
                SET_OBJECT_TINT_INDEX(selectedEntity.handle.Handle(), thisTextureVariation);
            }
        }
    }

    int thisHealth = selectedEntity.handle.GetHealth();
    bool bHealth_plus = false, bHealth_minus = false, bHealth_input = false;
    AddNumber(Language::Translate("menu.spooner.entity.option.health.label"), thisHealth, 0, bHealth_input, bHealth_plus, bHealth_minus);
    if (bHealth_plus)
    {
        if (thisHealth < INT_MAX)
        {
            thisHealth++;
            if (selectedEntity.handle.GetMaxHealth() < thisHealth)
            {
                selectedEntity.handle.SetMaxHealth(thisHealth);
            }
            selectedEntity.handle.SetHealth(thisHealth);
        }
    }
    if (bHealth_minus)
    {
        if (thisHealth > 0)
        {
            thisHealth--;
            if (selectedEntity.handle.GetMaxHealth() > thisHealth + 30)
            {
                selectedEntity.handle.SetMaxHealth(thisHealth + 30);
            }
            selectedEntity.handle.SetHealth(thisHealth);
        }
    }
    if (bHealth_input)
    {
        GTAentity& ent = selectedEntity.handle;
        std::string inputStr = Game::InputBox("", 5U, "", std::to_string(thisHealth));
        if (inputStr.length() > 0)
        {
            try
            {
                thisHealth = stoi(inputStr);
                if (ent.GetMaxHealth() < thisHealth)
                {
                    ent.SetMaxHealth(thisHealth);
                }
                ent.SetHealth(thisHealth);
            }
            catch (...)
            {
                Game::Print::PrintErrorInvalidInput(inputStr);
            }
        }
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SpoonerEoHealth, std::string(), 4U, std::string(), std::to_string(thisHealth));
        //OnscreenKeyboard::State::arg1._int = ent.GetHandle();
    }

    bool bInvinciblePressed = false;
    AddLocal(Language::Translate("menu.spooner.entity.option.invincible.label"), selectedEntity.handle.IsInvincible(), bInvinciblePressed, bInvinciblePressed);
    if (bInvinciblePressed)
    {
        bool bEnable = !selectedEntity.handle.IsInvincible();
        selectedEntity.handle.SetInvincible(bEnable);
        selectedEntity.handle.SetExplosionProof(bEnable);
        selectedEntity.handle.SetMeleeProof(bEnable);
    }

    bool bFireProofPressed = false;
    AddLocal(Language::Translate("menu.spooner.entity.option.fireproof.label"), selectedEntity.handle.IsFireProof(), bFireProofPressed, bFireProofPressed);
    if (bFireProofPressed)
    {
        bool bEnable = !selectedEntity.handle.IsFireProof();
        selectedEntity.handle.SetFireProof(bEnable);
    }
    bool bOnFirePressed = false;
    AddLocal(Language::Translate("menu.spooner.entity.option.on_fire.label"), selectedEntity.handle.IsOnFire(), bOnFirePressed, bOnFirePressed);
    if (bOnFirePressed)
    {
        bool bEnable = !selectedEntity.handle.IsOnFire();
        selectedEntity.handle.SetOnFire(bEnable);
    }

    bool bVisiblePressed = false;
    AddLocal(Language::Translate("menu.spooner.entity.option.visible.label"), selectedEntity.handle.IsVisible(), bVisiblePressed, bVisiblePressed);
    if (bVisiblePressed)
    {
        selectedEntity.handle.SetVisible(!selectedEntity.handle.IsVisible());
    }

    AddOption(Language::Translate("menu.spooner.entity.option.opacity.label"), null, SetSelectedEntityAsVehicleTarget, SUB::ENTITYALPHALEVEL);
    AddOptionDescription(Language::Translate("menu.spooner.entity.option.opacity.description"));

    bool bCollisionPressed = false;
    AddLocal(Language::Translate("menu.spooner.entity.option.collision.label"), selectedEntity.handle.GetIsCollisionEnabled(), bCollisionPressed, bCollisionPressed);
    if (bCollisionPressed)
    {
        GTAentity attBaseEnt;
        EntityManagement::GetEntityThisEntityIsAttachedTo(selectedEntity.handle, attBaseEnt);
        selectedEntity.handle.SetIsCollisionEnabled(!selectedEntity.handle.GetIsCollisionEnabled());
        if (selectedEntity.attachmentArgs.isAttached)
        {
            EntityManagement::AttachEntity(selectedEntity, attBaseEnt, selectedEntity.attachmentArgs.boneIndex, selectedEntity.attachmentArgs.offset, selectedEntity.attachmentArgs.rotation);
        }
    }

    bool bHasGravity = selectedEntity.handle.GetHasGravity();
    bool bGravityTogglePressed = false;
    AddLocal(Language::Translate("menu.spooner.entity.option.gravity.label"), bHasGravity, bGravityTogglePressed, bGravityTogglePressed);
    AddOptionDescription(Language::Translate("menu.spooner.entity.option.gravity.description"));
    if (bGravityTogglePressed)
    {
        switch (selectedEntity.type)
        {
        case EntityType::PED:
            GTAped(selectedEntity.handle).SetHasGravity(!bHasGravity);
            break;
        case EntityType::VEHICLE:
            GTAvehicle(selectedEntity.handle).SetHasGravity(!bHasGravity);
            break;
        case EntityType::PROP:
            GTAprop(selectedEntity.handle).SetHasGravity(!bHasGravity);
            break;
        default:
            selectedEntity.handle.SetHasGravity(!bHasGravity);
            break;
        }
        bHasGravity = !bHasGravity;
    }

    bool bGoToEntityPressed = false;
    AddOption(Language::Translate("menu.spooner.entity.option.go_to.label"), bGoToEntityPressed);
    if (bGoToEntityPressed)
    {
        if (SpoonerCamera::camera.IsActive())
        {
            auto& cam = SpoonerCamera::camera;
            cam.SetPosition(selectedEntity.handle.GetOffsetInWorldCoords(0, -5.0f - selectedEntity.handle.Dim2().y, 0));
        }
        else
        {
            GTAped myPed = PLAYER_PED_ID();
            myPed.SetPosition(selectedEntity.handle.GetPosition());
        }
    }

    bool bBringEntityToSelfPressed = false;
    AddOption(Language::Translate("menu.spooner.entity.option.bring_to_self.label"), bBringEntityToSelfPressed);
    if (selectedEntity.handle.IsAttached())
        AddOptionDescription(Language::Translate("menu.spooner.entity.option.bring_to_self.description"));
    if (bBringEntityToSelfPressed)
    {
        //GTAentity entityToTele = selectedEntity.handle;
        //GTAentity entityToTeleMaybe;
        //while (EntityManagement::GetEntityThisEntityIsAttachedTo(entityToTele, entityToTeleMaybe))
        //entityToTele = entityToTeleMaybe;
        if (selectedEntity.handle.IsAttached())
            EntityManagement::DetachEntity(selectedEntity); // Detach if attached :(

        if (SpoonerCamera::camera.IsActive())
        {
            auto& cam = SpoonerCamera::camera;
            selectedEntity.handle.SetPosition(cam.GetOffsetInWorldCoords(0, 5.0f + selectedEntity.handle.Dim2().y, 0));
        }
        else
        {
            GTAped myPed = PLAYER_PED_ID();
            selectedEntity.handle.SetPosition(myPed.GetPosition() + (myPed.ForwardVector() * (selectedEntity.handle.Dim1().y + 4.0f)));
        }
    }

    bool bPlaceOnGround = false;
    AddOption(Language::Translate("menu.spooner.entity.option.place_on_ground.label"), bPlaceOnGround);
    AddOptionDescription(Language::Translate("menu.spooner.entity.option.place_on_ground.description"));
    if (bPlaceOnGround)
    {
        selectedEntity.handle.PlaceOnGround();
    }

    bool bResetRotation = false;
    AddOption(Language::Translate("menu.spooner.entity.option.reset_rotation.label"), bResetRotation);
    if (bResetRotation)
    {
        selectedEntity.handle.SetRotation(Vector3::Zero());
    }

    AddOption(Language::Translate("menu.spooner.entity.option.trigger_fx.label"), null, SetSelectedEntityAsActivePed, SUB::PTFXSUB);

    bool bGoToTaskSeqMenu = false;
    AddTexter(
        Language::Translate("menu.spooner.entity.option.task_sequence.label"),
        selectedEntity.taskSequence.IsActive() ? 1 : 0,
        std::vector<std::string>{Language::Translate("menu.spooner.entity.option.task_sequence.value.inactive"), Language::Translate("menu.spooner.entity.option.task_sequence.value.active")},
        bGoToTaskSeqMenu
    );
    AddOptionDescription(Language::Translate("menu.spooner.entity.option.task_sequence.description"));
    if (bGoToTaskSeqMenu)
    {
        Menu::pendingSubmenu = SUB::SPOONER_TASKSEQUENCE_TASKLIST;
    }

    // peds can access anims from ped options menu
    if (selectedEntity.handle.IsVehicle() || selectedEntity.handle.IsProp())
    {
        AddOption(Language::Translate("menu.spooner.entity.option.animations.label"), null, SetSelectedEntityAsActivePed, SUB::ANIMATIONSUB);
    }

    if (selectedEntity.type == EntityType::PED)
    {
        AddOption(Language::Translate("menu.spooner.entity.option.ped_options.label"), null, nullFunc, SUB::SPOONER_PEDOPS);
    }
    else if (selectedEntity.type == EntityType::VEHICLE)
    {
        AddOption(Language::Translate("menu.spooner.entity.option.menyoo_customs.label"), null, SetSelectedEntityAsVehicleTarget, SUB::MODSHOP);
    }

    AddOption(Language::Translate("menu.spooner.entity.option.attachment_options.label"), null, nullFunc, SUB::SPOONER_ATTACHMENTOPS);
    AddOption(Language::Translate("menu.spooner.entity.option.manual_editing.label"), null, nullFunc, SUB::SPOONER_MANUALEDITING);
}

void Sub_AttachmentOps()
{
    if (!selectedEntity.handle.Exists())
    {
        Menu::SetPreviousMenu();
        return;
    }
    selectedEntity.handle.RequestControlOnce();
    auto thisEntityIndexInDb = EntityManagement::GetEntityIndexInDb(selectedEntity);
    bool isThisEntityInDb = thisEntityIndexInDb >= 0;
    if (isThisEntityInDb)
    {
        selectedEntity.taskSequence = Databases::EntityDb[thisEntityIndexInDb].taskSequence;
        Databases::EntityDb[thisEntityIndexInDb] = selectedEntity;
    }

    GTAentity parentEntity;
    bool seIsAttached = EntityManagement::GetEntityThisEntityIsAttachedTo(selectedEntity.handle, parentEntity);
    EntityType parentEntityType = (EntityType)parentEntity.Type();

    AddTitle(Language::Translate("menu.spooner.attachment.title"));

    if (!seIsAttached)
    {
        AddOption(Language::Translate("menu.spooner.attachment.option.attach_to.label"), null, nullFunc, SUB::SPOONER_ATTACHMENTOPS_ATTACHTO);
    }
    else
    {
        bool bDetachPressed = false;
        AddOption(Language::Translate("menu.spooner.attachment.option.detach.label"), bDetachPressed);
        if (bDetachPressed)
        {
            EntityManagement::DetachEntity(selectedEntity);
        }
    }

    if (seIsAttached)
    {
        int nextBoneIndex = selectedEntity.attachmentArgs.boneIndex;
        Vector3 nextOffset = selectedEntity.attachmentArgs.offset;
        Vector3 nextRot = selectedEntity.attachmentArgs.rotation;

        SpoonerMode::UpdateEntityEditingState(nextOffset, nextRot);

        SpoonerMode::editingState.transformMode = static_cast<SpoonerMode::eTransformMode>(AddTexterCycler(
            Language::Translate("menu.spooner.editing.option.mode.label"),
            (int)SpoonerMode::editingState.transformMode > 1 ? 0 : (int)SpoonerMode::editingState.transformMode,
            {Language::Translate("menu.spooner.option.editing.value.position"), Language::Translate("menu.spooner.option.editing.value.rotation")}
        ));
        AddOptionDescription(Language::Translate("menu.spooner.editing.option.position_rotation.description"));

        // Bone text scroller if type is PED or VEHICLE
        if (parentEntityType == EntityType::PED)
        {
            int obj_currentPedBoneArrayIndex = 17;
            for (int i = 0; i < Bone::vBoneNames.size(); i++)
            {
                if (nextBoneIndex == GTAped(parentEntity).GetBoneIndex(Bone::vBoneNames[i].boneid))
                {
                    obj_currentPedBoneArrayIndex = i;
                    break;
                }
            }
            std::vector<std::string> vCurrentPedBoneNameStr{Bone::vBoneNames[obj_currentPedBoneArrayIndex].name};

            bool pbone_plus = 0, pbone_minus = 0, pbone_input = 0;
            AddTexter(Language::Translate("menu.spooner.attachment.option.bone.label"), 0, vCurrentPedBoneNameStr, pbone_input, pbone_plus, pbone_minus);
            if (pbone_plus)
            {
                if (obj_currentPedBoneArrayIndex < Bone::vBoneNames.size() - 1)
                {
                    obj_currentPedBoneArrayIndex++;
                    nextBoneIndex = GTAped(parentEntity).GetBoneIndex(Bone::vBoneNames[obj_currentPedBoneArrayIndex].boneid);
                }
            }
            if (pbone_minus)
            {
                if (obj_currentPedBoneArrayIndex > 0)
                {
                    obj_currentPedBoneArrayIndex--;
                    nextBoneIndex = GTAped(parentEntity).GetBoneIndex(Bone::vBoneNames[obj_currentPedBoneArrayIndex].boneid);
                }
            }
            if (pbone_input)
            {
                Menu::pendingSubmenu = SUB::SPOONER_ATTACHMENTOPS_SELECTBONE;
            }
        }
        else if (parentEntityType == EntityType::VEHICLE)
        {
            int obj_currentVehBoneArrayIndex = 10;
            for (int i = 0; i < VBone::vNames.size(); i++)
            {
                if (nextBoneIndex == GTAvehicle(parentEntity).GetBoneIndex(VBone::vNames[i]))
                {
                    obj_currentVehBoneArrayIndex = i;
                    break;
                }
            }
            std::vector<std::string> vCurrentVehBoneNameStr{VBone::vNames[obj_currentVehBoneArrayIndex]};

            bool vbone_plus = 0, vbone_minus = 0, vbone_input = 0;
            AddTexter(Language::Translate("menu.spooner.attachment.option.bone.label"), 0, vCurrentVehBoneNameStr, vbone_input, vbone_plus, vbone_minus);
            if (vbone_plus)
            {
                if (obj_currentVehBoneArrayIndex < VBone::vNames.size() - 1)
                {
                    obj_currentVehBoneArrayIndex++;
                    nextBoneIndex = GTAvehicle(parentEntity).GetBoneIndex(VBone::vNames[obj_currentVehBoneArrayIndex]);
                }
            }
            if (vbone_minus)
            {
                if (obj_currentVehBoneArrayIndex > 0)
                {
                    obj_currentVehBoneArrayIndex--;
                    nextBoneIndex = GTAvehicle(parentEntity).GetBoneIndex(VBone::vNames[obj_currentVehBoneArrayIndex]);
                }
            }
            if (vbone_input)
            {
                Menu::pendingSubmenu = SUB::SPOONER_ATTACHMENTOPS_SELECTBONE;
            }
        }

        AddBreak(Language::Translate("menu.spooner.editing.section.values.label"));

        auto& precision = SpoonerMode::editingState.transformMode == SpoonerMode::eTransformMode::Position ? SpoonerMode::editingState.precisionPos : SpoonerMode::editingState.precisionRot;

        AddNumberMultiplier(Language::Translate("menu.spooner.editing.option.scroll_sensitivity.label"), precision, 4, 10.0, 0.0001, 10.0, !Settings::bInvertScrollSensitivity);
        AddOptionDescription(Language::Translate("menu.spooner.editing.option.scroll_sensitivity.description"));

        switch (SpoonerMode::editingState.transformMode)
        {
        case SpoonerMode::eTransformMode::Position:
        {
            AddNumberStepper(Language::Translate("common.axis.x"), nextOffset.x, 4, (double)precision);
            AddNumberStepper(Language::Translate("common.axis.y"), nextOffset.y, 4, (double)precision);
            AddNumberStepper(Language::Translate("common.axis.z"), nextOffset.z, 4, (double)precision);
            break;
        }
        case SpoonerMode::eTransformMode::Rotation:
        {
            bool bResetRot = false;
            AddNumberStepper(Language::Translate("menu.spooner.editing.option.pitch.label"), nextRot.x, 4, (double)precision);
            AddNumberStepper(Language::Translate("menu.spooner.editing.option.roll.label"), nextRot.y, 4, (double)precision);
            AddNumberStepper(Language::Translate("menu.spooner.editing.option.yaw.label"), nextRot.z, 4, (double)precision);
            AddOption(Language::Translate("menu.spooner.editing.option.reset_rotation.label"), bResetRot);
            if (bResetRot)
                nextRot = Vector3();
            break;
        }
        }


        WrapAngle(nextRot.x);
        WrapAngle(nextRot.y);
        WrapAngle(nextRot.z);

        if (nextOffset != selectedEntity.attachmentArgs.offset || nextRot != selectedEntity.attachmentArgs.rotation || nextBoneIndex != selectedEntity.attachmentArgs.boneIndex)
        {
            EntityManagement::AttachEntity(selectedEntity, parentEntity, nextBoneIndex, nextOffset, nextRot);
        }

        AddBreak(Language::Translate("menu.spooner.editing.section.options.label"));
        AddOption(Language::Translate("menu.spooner.editing.option.snapping.label"), null, nullFunc, SUB::SPOONER_MANUALEDITING_SNAP);

        SpoonerMode::editingState.SetMode(
            static_cast<SpoonerMode::eEditMode>(AddTexterCycler(
                Language::Translate("menu.spooner.editing.option.manipulation_mode.label"),
                (int)SpoonerMode::editingState.mode,
                {Language::Translate("menu.spooner.option.editing.value.none"),
                 Language::Translate("menu.spooner.option.editing.value.keyboard"),
                 Language::Translate("menu.spooner.option.editing.value.gizmo")}
            ))
        );
        AddOptionDescription(Language::Translate("menu.spooner.editing.option.manipulation_mode.description"));
        if (SpoonerMode::editingState.mode != SpoonerMode::eEditMode::Disabled && SpoonerMode::editingState.transformMode != SpoonerMode::eTransformMode::Scale)
        {
            AddToggle(Language::Translate("menu.spooner.editing.option.local_space.label"), SpoonerMode::editingState.localSpace);
            AddOptionDescription(Language::Translate("menu.spooner.editing.option.local_space.description"));
        }
    }
}

void Sub_AttachmentOps_AttachTo()
{
    if (!selectedEntity.handle.Exists())
    {
        Menu::SetPreviousMenu();
        return;
    }
    GTAped myPed = PLAYER_PED_ID();
    GTAvehicle myVehicle = g_myVeh;

    AddTitle(Language::Translate("menu.spooner.attachment.attach_to.title"));

    bool bToggleKeepPosWhenAttaching = false;
    AddTickol(
        Language::Translate("menu.spooner.attachment.option.keep_world_position.label"),
        Settings::bKeepPositionWhenAttaching,
        bToggleKeepPosWhenAttaching,
        bToggleKeepPosWhenAttaching,
        TICKOL::BOXTICK,
        TICKOL::BOXBLANK
    );
    AddOptionDescription(Language::Translate("menu.spooner.attachment.option.keep_world_position.description"));
    if (bToggleKeepPosWhenAttaching)
        Settings::bKeepPositionWhenAttaching = !Settings::bKeepPositionWhenAttaching;

    AddBreak(Language::Translate("menu.spooner.attachment.section.available_entities.label"));

    bool bSelf = false;
    AddOption(Language::Translate("menu.spooner.attachment.option.self.label"), bSelf);
    if (bSelf)
    {
        EntityManagement::AttachEntityInit(selectedEntity, myPed, Settings::bKeepPositionWhenAttaching);
        Menu::SetPreviousMenu();
        return;
    }

    if (myVehicle.Exists())
    {
        bool bSelfVeh = false;
        AddOption(Language::Translate(myPed.IsInVehicle() ? "menu.spooner.attachment.option.current_vehicle.label" : "menu.spooner.attachment.option.last_seated_vehicle.label"), bSelfVeh);
        if (bSelfVeh)
        {
            EntityManagement::AttachEntityInit(selectedEntity, myVehicle, Settings::bKeepPositionWhenAttaching);
            Menu::SetPreviousMenu();
            return;
        }
    }

    if (!Databases::EntityDb.empty())
    {
        if (Databases::EntityDb.size() > 1 || Databases::EntityDb.front().handle != selectedEntity.handle)
        {
            AddBreak(Language::Translate("menu.spooner.attachment.section.database.label"));
            for (auto& e : Databases::EntityDb)
            {
                if (e.handle != selectedEntity.handle)
                {
                    if (e.handle.Exists())
                    {
                        if (!selectedEntity.handle.IsAttachedTo(e.handle))
                        {
                            bool bEntityPressed = false;
                            AddOption(e.hashName, bEntityPressed);
                            if (bEntityPressed)
                            {
                                EntityManagement::AttachEntityInit(selectedEntity, e.handle, Settings::bKeepPositionWhenAttaching);
                                Menu::SetPreviousMenu();
                                return;
                            }
                            if (Menu::IsLastDrawnOptionSelected())
                                EntityManagement::ShowArrowAboveEntity(e.handle, RGBA(0, 255, 0, 200));
                        }
                        else
                        {
                            AddOption(e.hashName + " (already attached)", null);
                            if (Menu::IsLastDrawnOptionSelected())
                                EntityManagement::ShowArrowAboveEntity(e.handle, RGBA(255, 0, 0, 200));
                        }
                    }
                    else
                    {
                        AddOption(e.hashName + " (Invalid)", null);
                    }
                }
            }
        }
    }
}

void Sub_AttachmentOps_SelectBone()
{
    if (!selectedEntity.handle.Exists())
    {
        Menu::SetPreviousMenu();
        Menu::selectedOptionIndex = 1;
        return;
    }
    selectedEntity.handle.RequestControlOnce();
    auto thisEntityIndexInDb = EntityManagement::GetEntityIndexInDb(selectedEntity);
    bool isThisEntityInDb = thisEntityIndexInDb >= 0;
    if (isThisEntityInDb)
    {
        selectedEntity.taskSequence = Databases::EntityDb[thisEntityIndexInDb].taskSequence;
        Databases::EntityDb[thisEntityIndexInDb] = selectedEntity;
    }

    GTAentity baseEntity;
    bool seIsAttached = EntityManagement::GetEntityThisEntityIsAttachedTo(selectedEntity.handle, baseEntity);
    EntityType parentEntityType = (EntityType)baseEntity.Type();
    if (!baseEntity.Exists())
    {
        Menu::SetPreviousMenu();
        Menu::selectedOptionIndex = 1;
        return;
    }

    int nextBoneIndex = selectedEntity.attachmentArgs.boneIndex;
    Vector3 nextOffset = selectedEntity.attachmentArgs.offset;
    Vector3 nextRot = selectedEntity.attachmentArgs.rotation;

    bool bSelectedBoneIndexExists = false; // Idek what this is doing here
    int defaultBone;

    AddTitle(Language::Translate("menu.spooner.attachment.bone.title"));

    // Bone text scroller if type is PED or VEHICLE. Reattach and reset args on bone change.
    if (parentEntityType == EntityType::PED)
    {
        GTAped baseEntityPed = baseEntity;
        defaultBone = baseEntityPed.GetBoneIndex(Bone::SKEL_ROOT);

        bSelectedBoneIndexExists = nextBoneIndex == baseEntityPed.GetBoneIndex(Bone::vBoneNames.front().boneid);
        for (auto& b : Bone::vBoneNames)
        {
            bool bBoneNamePressed = false;
            if (bSelectedBoneIndexExists && nextBoneIndex == baseEntityPed.GetBoneIndex(b.boneid))
            {
                continue;
            }
            bSelectedBoneIndexExists = (nextBoneIndex == baseEntityPed.GetBoneIndex(b.boneid)); // And is selected (in this case)
            AddTickol(b.name, bSelectedBoneIndexExists, bBoneNamePressed, bBoneNamePressed);
            if (bBoneNamePressed)
            {
                nextBoneIndex = baseEntityPed.GetBoneIndex(b.boneid);
                //nextOffset = Vector3::Zero();
                //nextRot = Vector3::Zero();
                break;
            }
        }
    }
    else if (parentEntityType == EntityType::VEHICLE)
    {
        GTAvehicle baseEntityVeh = baseEntity;
        defaultBone = baseEntityVeh.GetBoneIndex(VBone::bodyshell);

        bSelectedBoneIndexExists = nextBoneIndex == baseEntityVeh.GetBoneIndex(VBone::vNames.front());
        for (auto& b : VBone::vNames)
        {
            bool bBoneNamePressed = false;
            if (!baseEntityVeh.HasBone(b) || (bSelectedBoneIndexExists && nextBoneIndex == baseEntityVeh.GetBoneIndex(b)))
            {
                continue;
            }
            bSelectedBoneIndexExists = (nextBoneIndex == baseEntityVeh.GetBoneIndex(b)); // And is selected (in this case)
            AddTickol(b, bSelectedBoneIndexExists, bBoneNamePressed, bBoneNamePressed);
            if (bBoneNamePressed)
            {
                nextBoneIndex = baseEntityVeh.GetBoneIndex(b);
                //nextOffset = Vector3::Zero();
                //nextRot = Vector3::Zero();
                break;
            }
        }
    }

    if (nextBoneIndex != selectedEntity.attachmentArgs.boneIndex)
    {
        EntityManagement::AttachEntity(selectedEntity, baseEntity, nextBoneIndex, nextOffset, nextRot);
    }
}

void Sub_Snapping()
{
    AddTitle(Language::Translate("menu.spooner.snapping.title"));

    AddToggle(Language::Translate("menu.spooner.snapping.option.grid_snap.label"), Settings::bGridSnapEnabled);
    AddOptionDescription(Language::Translate("menu.spooner.snapping.option.grid_snap.description"));
    AddNumberStepper(Language::Translate("menu.spooner.snapping.option.grid_size.label"), Settings::gridSnapSize, 2, 0.25, 0.01);
    AddOptionDescription(Language::Translate("menu.spooner.snapping.option.grid_size.description"));
    AddNumberStepper(Language::Translate("menu.spooner.snapping.option.rotation_snap.label"), Settings::rotationSnapDegrees, 1, 5.0, 0.0);
    AddOptionDescription(Language::Translate("menu.spooner.snapping.option.rotation_snap.description"));
    AddToggle(Language::Translate("menu.spooner.snapping.option.ground.label"), Settings::bSnapToGround);
    AddOptionDescription(Language::Translate("menu.spooner.snapping.option.ground.description"));
    AddToggle(Language::Translate("menu.spooner.snapping.option.draw_grid.label"), Settings::bDrawGrid);
}

void Sub_ManualEditing()
{
    if (!selectedEntity.handle.Exists())
    {
        Menu::SetPreviousMenu();
        return;
    }
    if (selectedEntity.handle.IsAttached())
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.editing.attached_entity_error.message"));
        Menu::SetPreviousMenu();
        return;
    }
    selectedEntity.handle.RequestControlOnce();

    AddTitle(Language::Translate("menu.spooner.editing.title"));

    Vector3 currPos = selectedEntity.handle.GetPosition();
    Vector3 currRot = selectedEntity.handle.GetRotation();

    Entity handle = selectedEntity.handle.Handle();
    auto& scaleState = IS_ENTITY_A_VEHICLE(handle) ? _vehScale : IS_ENTITY_A_PED(handle) ? _pedScale : _objScale;

    Vector3 nextPos = currPos;
    Vector3 nextRot = currRot;
    Vector3 nextScale = scaleState.scale;

    SpoonerMode::UpdateEntityEditingState(nextPos, nextRot);
    // Mode selector
    SpoonerMode::editingState.transformMode = static_cast<SpoonerMode::eTransformMode>(AddTexterCycler(
        Language::Translate("menu.spooner.editing.option.mode.label"),
        static_cast<int>(SpoonerMode::editingState.transformMode),
        {Language::Translate("menu.spooner.option.editing.value.position"),
         Language::Translate("menu.spooner.option.editing.value.rotation"),
         Language::Translate("menu.spooner.option.editing.value.scale")}
    ));
    AddOptionDescription(Language::Translate("menu.spooner.editing.option.position_rotation_scale.description"));

    AddBreak(Language::Translate("menu.spooner.editing.section.values.label"));

    float& precision = SpoonerMode::editingState.transformMode == SpoonerMode::eTransformMode::Position   ? SpoonerMode::editingState.precisionPos
                       : SpoonerMode::editingState.transformMode == SpoonerMode::eTransformMode::Rotation ? SpoonerMode::editingState.precisionRot
                                                                                                          : SpoonerMode::editingState.precisionScale;

    AddNumberMultiplier(Language::Translate("menu.spooner.editing.option.scroll_sensitivity.label"), precision, 4, 10.0, 0.0001, 10.0, !Settings::bInvertScrollSensitivity);
    AddOptionDescription(Language::Translate("menu.spooner.editing.option.scroll_sensitivity.description"));

    switch (SpoonerMode::editingState.transformMode)
    {
    case SpoonerMode::eTransformMode::Position:
    {
        AddNumberStepper(Language::Translate("common.axis.x"), nextPos.x, 4, (double)precision);
        AddNumberStepper(Language::Translate("common.axis.y"), nextPos.y, 4, (double)precision);
        AddNumberStepper(Language::Translate("common.axis.z"), nextPos.z, 4, (double)precision);
        break;
    }
    case SpoonerMode::eTransformMode::Rotation:
    {
        bool bResetRot = false;
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.pitch.label"), nextRot.x, 4, (double)precision);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.roll.label"), nextRot.y, 4, (double)precision);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.yaw.label"), nextRot.z, 4, (double)precision);
        AddOption(Language::Translate("menu.spooner.editing.option.reset_rotation.label"), bResetRot);
        if (bResetRot)
            nextRot = Vector3();
        break;
    }
    case SpoonerMode::eTransformMode::Scale:
    {
        bool bResetScale = false;
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.width.label"), nextScale.x, 4, (double)precision, 0.01);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.length.label"), nextScale.y, 4, (double)precision, 0.01);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.height.label"), nextScale.z, 4, (double)precision, 0.01);
        AddOption(Language::Translate("menu.spooner.editing.option.reset_scale.label"), bResetScale);
        if (bResetScale)
            nextScale = Vector3(1.0f, 1.0f, 1.0f);
        break;
    }
    }

    AddBreak(Language::Translate("menu.spooner.editing.section.options.label"));
    AddOption(Language::Translate("menu.spooner.editing.option.snapping.label"), null, nullFunc, SUB::SPOONER_MANUALEDITING_SNAP);


    SpoonerMode::editingState.SetMode(
        static_cast<SpoonerMode::eEditMode>(AddTexterCycler(
            Language::Translate("menu.spooner.editing.option.manipulation_mode.label"),
            (int)SpoonerMode::editingState.mode,
            {Language::Translate("menu.spooner.option.editing.value.none"),
             Language::Translate("menu.spooner.option.editing.value.keyboard"),
             Language::Translate("menu.spooner.option.editing.value.gizmo")}
        ))
    );
    AddOptionDescription(Language::Translate("menu.spooner.editing.option.manipulation_mode.description"));

    // don't show if not in editing mode or if in scale mode (because scaling is always local-space)
    if (SpoonerMode::editingState.mode != SpoonerMode::eEditMode::Disabled && SpoonerMode::editingState.transformMode != SpoonerMode::eTransformMode::Scale)
    {
        AddToggle(Language::Translate("menu.spooner.editing.option.local_space.label"), SpoonerMode::editingState.localSpace);
        AddOptionDescription(Language::Translate("menu.spooner.editing.option.local_space.description"));
    }


    // Apply position changes
    if (nextPos != currPos)
    {
        selectedEntity.handle.SetPosition(SpoonerMode::SnapPos(nextPos));
        currPos = selectedEntity.handle.GetPosition();
        GTAentity attBase;
        if (EntityManagement::GetEntityThisEntityIsAttachedTo(selectedEntity.handle, attBase))
            World::DrawLine(attBase.GetPosition(), currPos, RGBA::AllWhite());
    }
    // Apply rotation changes
    if (nextRot != currRot)
    {
        WrapAngle(nextRot.x);
        WrapAngle(nextRot.y);
        WrapAngle(nextRot.z);
        selectedEntity.handle.SetRotation(SpoonerMode::SnapRot(nextRot));
        currRot = selectedEntity.handle.GetRotation();
        GTAentity attBase;
        if (EntityManagement::GetEntityThisEntityIsAttachedTo(selectedEntity.handle, attBase))
            World::DrawLine(attBase.GetPosition(), currPos, RGBA::AllWhite());
    }
    // Apply scale changes
    if (nextScale != scaleState.scale)
    {
        if (scaleState.handle != handle)
            scaleState.handle = handle;
        scaleState.scale = nextScale;
        selectedEntity.handle.SetScale(nextScale);
        if (!IS_ENTITY_A_VEHICLE(handle) && !IS_ENTITY_A_PED(handle))
        {
            selectedEntity.handle.SetIsCollisionEnabled(false);
            selectedEntity.handle.FreezePosition(true);
        }
    }
}

void Sub_Vector3_ManualEditing()
{
    auto& ptrs = SpoonerVector3ManualEditingPtrs;
    bool hasPos = std::get<1>(ptrs) != nullptr;
    bool hasRot = std::get<2>(ptrs) != nullptr;

    if (!hasPos && !hasRot)
    {
        Menu::SetPreviousMenu();
        return;
    }

    // Draw debug marker at position
    if (hasPos)
    {
        Vector3 markerPos = *std::get<1>(ptrs);
        if (std::get<0>(ptrs).Exists())
        {
            markerPos = std::get<0>(ptrs).GetOffsetInWorldCoords(markerPos);
        }
        World::DrawLightWithRange(markerPos, g_fadedRGB, 2.3f, 1.5f);
        World::DrawMarker(MarkerType::DebugSphere, markerPos, Vector3(), Vector3(), Vector3(0.1f, 0.1f, 0.1f), g_fadedRGB.ToRGBA(190));
    }

    AddTitle(Language::Translate("menu.spooner.editing.vector_title.title"));

    // Clamp editing mode to available ptrs
    auto& editMode = SpoonerMode::editingState.transformMode;
    if ((!hasPos && editMode == SpoonerMode::eTransformMode::Position) || (!hasRot && editMode == SpoonerMode::eTransformMode::Rotation))
    {
        editMode = hasPos ? SpoonerMode::eTransformMode::Position : SpoonerMode::eTransformMode::Rotation;
    }

    // Transform mode cycler (Position / Rotation only)
    {
        std::vector<std::string> modeNames;
        if (hasPos)
            modeNames.push_back(Language::Translate("menu.spooner.editing.option.position.label"));
        if (hasRot)
            modeNames.push_back(Language::Translate("menu.spooner.editing.option.rotation.label"));
        if (modeNames.size() > 1)
        {
            editMode = static_cast<SpoonerMode::eTransformMode>(AddTexterCycler(Language::Translate("menu.spooner.editing.option.mode.label"), static_cast<int>(editMode), modeNames));
            AddOptionDescription(Language::Translate("menu.spooner.editing.option.position_rotation.description"));
        }
    }

    AddBreak(Language::Translate("menu.spooner.editing.section.values.label"));

    float& precision = editMode == SpoonerMode::eTransformMode::Position ? SpoonerMode::editingState.precisionPos : SpoonerMode::editingState.precisionRot;

    AddNumberMultiplier(Language::Translate("menu.spooner.editing.option.scroll_sensitivity.label"), precision, 4, 10.0, 0.0001, 10.0, !Settings::bInvertScrollSensitivity);
    AddOptionDescription(Language::Translate("menu.spooner.editing.option.scroll_sensitivity.description"));

    if (editMode == SpoonerMode::eTransformMode::Position)
    {
        Vector3& pos = *std::get<1>(ptrs);
        AddNumberStepper(Language::Translate("common.axis.x"), pos.x, 4, (double)precision);
        AddNumberStepper(Language::Translate("common.axis.y"), pos.y, 4, (double)precision);
        AddNumberStepper(Language::Translate("common.axis.z"), pos.z, 4, (double)precision);
    }
    else
    {
        Vector3& rot = *std::get<2>(ptrs);
        bool bResetRot = false;
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.pitch.label"), rot.x, 4, (double)precision);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.roll.label"), rot.y, 4, (double)precision);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.yaw.label"), rot.z, 4, (double)precision);
        AddOption(Language::Translate("menu.spooner.editing.option.reset_rotation.label"), bResetRot);
        if (bResetRot)
            rot = Vector3();

        WrapAngle(rot.x);
        WrapAngle(rot.y);
        WrapAngle(rot.z);
    }
}

void Sub_MultiSelect()
{
    // show in-game indicator of which entities are selected
    for (const auto& entity : MultiSelect::g_selectedEntities)
    {
        if (entity.handle.Exists())
            EntityManagement::ShowArrowAboveEntity(entity.handle, RGBA(127, 0, 255, 200));
    }

    if (!MultiSelect::g_bulkEditActive)
    {
        MultiSelect::g_prevSelected = selectedEntity;
        MultiSelect::g_bulkEditActive = true;
    }

    // Create pivot at centroid if entities are selected but no pivot exists yet
    if (!MultiSelect::g_groupPivot.Exists() && !MultiSelect::g_selectedEntities.empty())
        MultiSelect::CreatePivot();

    // Point gizmo at pivot
    if (MultiSelect::g_groupPivot.Exists())
    {
        selectedEntity.handle = MultiSelect::g_groupPivot;
        selectedEntity.attachmentArgs.isAttached = false;
    }

    // Restore state on sub-back
    if (Menu::OnSubBack == nullptr)
    {
        Menu::OnSubBack = []()
        {
            if (MultiSelect::g_bulkEditActive)
            {
                selectedEntity = MultiSelect::g_prevSelected;
                MultiSelect::g_bulkEditActive = false;
            }
            MultiSelect::DestroyPivot();
        };
    }

    AddTitle(Language::Translate("menu.spooner.multi_select.title"));

    bool bSelectAll = false, bClearAll = false;
    static const std::vector<std::string> copyModeOptions = {
        Language::Translate("menu.spooner.multi_select.option.selected_only.label"), Language::Translate("menu.spooner.multi_select.option.copy_with_attachments.label")
    };
    std::string graySelectAllPrefix = MultiSelect::g_selectedEntities.size() < Databases::EntityDb.size() ? "" : "~c~";
    std::string dbEntityCount = std::to_string(Databases::EntityDb.size());
    AddOption(graySelectAllPrefix + Language::Format("menu.spooner.multi_select.option.select_all.label", {{"count", dbEntityCount}}), bSelectAll);
    if (bSelectAll && MultiSelect::g_selectedEntities.size() < Databases::EntityDb.size())
    {
        MultiSelect::DestroyPivot();
        MultiSelect::SelectAll();
        if (MultiSelect::g_bulkEditActive)
        {
            selectedEntity = MultiSelect::g_prevSelected;
            MultiSelect::g_bulkEditActive = false;
        }
        *Menu::activeOptionIndex = 1;
        return;
    }

    std::string grayClearPrefix = MultiSelect::g_selectedEntities.empty() ? "~c~" : "";
    std::string selectedEntityCount = std::to_string(MultiSelect::g_selectedEntities.size());
    AddOption(grayClearPrefix + Language::Format("menu.spooner.multi_select.option.clear_selection.label", {{"count", selectedEntityCount}}), bClearAll);
    if (bClearAll && !MultiSelect::g_selectedEntities.empty())
    {
        MultiSelect::DestroyPivot();
        MultiSelect::Clear();
        if (MultiSelect::g_bulkEditActive)
        {
            selectedEntity = MultiSelect::g_prevSelected;
            MultiSelect::g_bulkEditActive = false;
        }
        *Menu::activeOptionIndex = 1;
        return;
    }
    bool bCopyPressed = false;
    _copyEntTexterValue = (UINT8)AddTexterCycler(
        (MultiSelect::g_selectedEntities.empty() ? "~c~" + Language::Translate("menu.spooner.entity.option.copy.label") + "~s~" : Language::Translate("menu.spooner.entity.option.copy.label")),
        _copyEntTexterValue,
        copyModeOptions,
        bCopyPressed
    );
    AddOptionDescription(Language::Translate("menu.spooner.entity.option.copy.description"));
    if (bCopyPressed)
    {
        for (auto& e : MultiSelect::g_selectedEntities)
        {
            SpoonerEntity copy = EntityManagement::CopyEntity(e, true, true, _copyEntTexterValue);
            MultiSelect::ApplySavedEntityState(e.handle, copy.handle);
        }
    }

    if (!Databases::EntityDb.empty())
    {
        AddBreak(Language::Translate("menu.spooner.multi_select.section.entities.label"));

        for (auto& e : Databases::EntityDb)
        {
            if (!e.handle.Exists())
            {
                AddOption(Language::Format("menu.spooner.multi_select.option.invalid_entity.label", {{"name", e.hashName}}), null);
                continue;
            }

            bool bInMultiSelect = MultiSelect::IsSelected(e.handle);
            // selected entities are being attached to the pivot, so only unselected ones can be genuinely attached
            bool bBlocked = !bInMultiSelect && e.handle.IsAttached();
            bool bEntityPressed = false;
            const std::string entityLabel = bBlocked ? Language::Format("menu.spooner.multi_select.option.attached_entity.label", {{"name", e.hashName}}) : e.hashName;
            AddTickol(entityLabel, bInMultiSelect, bEntityPressed, bEntityPressed, TICKOL::BOXTICK, TICKOL::BOXBLANK);
            if (bBlocked)
                AddOptionDescription(Language::Translate("menu.spooner.multi_select.option.attached_description.description"));
            if (Menu::IsLastDrawnOptionSelected())
                EntityManagement::ShowArrowAboveEntity(e.handle, RGBA(127, 0, 255, 200));

            if (bEntityPressed && !bBlocked)
            {
                MultiSelect::DestroyPivot();
                if (bInMultiSelect)
                    MultiSelect::Remove(e.handle);
                else
                    MultiSelect::Add(e);
            }
        }
    }


    // Display pivot position and rotation and allow editing of all selected entities
    if (!MultiSelect::g_selectedEntities.empty() && MultiSelect::g_groupPivot.Exists())
    {
        bool isOnTheLine = NETWORK_IS_IN_SESSION() != 0;
        bool bDelete = false;
        Vector3 pivotPos = MultiSelect::g_groupPivot.GetPosition();
        Vector3 pivotRot = MultiSelect::g_groupPivot.GetRotation();
        Vector3 basePos = pivotPos;
        Vector3 baseRot = pivotRot;

        float& precisionPos = SpoonerMode::editingState.precisionPos;
        float& precisionRot = SpoonerMode::editingState.precisionRot;
        float opacityDelta = 0.0f;

        SpoonerMode::UpdateEntityEditingState(pivotPos, pivotRot);

        AddBreak(Language::Translate("menu.spooner.multi_select.section.bulk_edit.label"));

        AddOption(Language::Translate("menu.spooner.entity.option.delete.label"), bDelete);
        AddNumberMultiplier(Language::Translate("menu.spooner.multi_select.option.scroll_position.label"), precisionPos, 4, 10.0, 0.0001, 10.0, !Settings::bInvertScrollSensitivity);
        AddOptionDescription(Language::Translate("menu.spooner.multi_select.option.scroll_position.description"));
        AddNumberMultiplier(Language::Translate("menu.spooner.multi_select.option.scroll_rotation.label"), precisionRot, 4, 10.0, 0.0001, 10.0, !Settings::bInvertScrollSensitivity);
        AddOptionDescription(Language::Translate("menu.spooner.multi_select.option.scroll_rotation.description"));

        AddNumberStepper(Language::Translate("menu.spooner.multi_select.option.pos_x.label"), pivotPos.x, 4, (double)precisionPos);
        AddNumberStepper(Language::Translate("menu.spooner.multi_select.option.pos_y.label"), pivotPos.y, 4, (double)precisionPos);
        AddNumberStepper(Language::Translate("menu.spooner.multi_select.option.pos_z.label"), pivotPos.z, 4, (double)precisionPos);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.pitch.label"), pivotRot.x, 4, (double)precisionRot);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.roll.label"), pivotRot.y, 4, (double)precisionRot);
        AddNumberStepper(Language::Translate("menu.spooner.editing.option.yaw.label"), pivotRot.z, 4, (double)precisionRot);
        AddNumberStepper(Language::Translate("menu.spooner.entity.option.opacity.label"), opacityDelta, 0, 1.0);
        AddOptionDescription(Language::Translate("menu.spooner.multi_select.option.opacity.description"));

        if (bDelete)
        {
            for (auto& e : MultiSelect::g_selectedEntities)
            {
                if (!e.handle.Exists())
                    continue;
                if (isOnTheLine)
                    e.handle.RequestControl();
                EntityManagement::DeleteEntity(e);
            }
            MultiSelect::Clear();
        }

        // Apply pivot pos/rot to pivot itself
        if (pivotPos.x != basePos.x || pivotPos.y != basePos.y || pivotPos.z != basePos.z)
        {
            if (isOnTheLine)
                MultiSelect::g_groupPivot.RequestControl();
            MultiSelect::g_groupPivot.SetPosition(SpoonerMode::SnapPos(pivotPos));
        }
        if (pivotRot.x != baseRot.x || pivotRot.y != baseRot.y || pivotRot.z != baseRot.z)
        {
            WrapAngle(pivotRot.x);
            WrapAngle(pivotRot.y);
            WrapAngle(pivotRot.z);
            if (isOnTheLine)
                MultiSelect::g_groupPivot.RequestControl();
            MultiSelect::g_groupPivot.SetRotation(SpoonerMode::SnapRot(pivotRot));
        }

        // Apply opacity delta to all selected entities
        if (opacityDelta != 0.0f)
        {
            for (auto& e : MultiSelect::g_selectedEntities)
            {
                if (!e.handle.Exists())
                    continue;
                if (isOnTheLine)
                    e.handle.RequestControl();
                int alpha = e.handle.GetAlpha() + static_cast<int>(opacityDelta);
                e.handle.SetAlpha(alpha);
            }
        }

        AddBreak(Language::Translate("menu.spooner.multi_select.section.task_sequences.label"));

        bool bStartTaskSequences = false;
        AddOption(Language::Translate("menu.spooner.multi_select.option.start_task_sequences.label"), bStartTaskSequences);
        if (bStartTaskSequences)
        {
            for (auto& e : MultiSelect::g_selectedEntities)
            {
                auto eiidb = EntityManagement::GetEntityIndexInDb(e);
                if (eiidb >= 0)
                {
                    if (isOnTheLine)
                        e.handle.RequestControl();
                    Databases::EntityDb[eiidb].taskSequence.Start();
                }
            }
        }

        bool bStopTaskSequences = false;
        AddOption(Language::Translate("menu.spooner.multi_select.option.stop_task_sequences.label"), bStopTaskSequences);
        if (bStopTaskSequences)
        {
            for (auto& e : MultiSelect::g_selectedEntities)
            {
                auto eiidb = EntityManagement::GetEntityIndexInDb(e);
                if (eiidb >= 0)
                {
                    if (e.handle.IsPed())
                    {
                        if (isOnTheLine)
                            e.handle.RequestControl();
                        TASK_CLEAR_LOOK_AT(e.handle.Handle());
                        CLEAR_PED_TASKS_IMMEDIATELY(e.handle.Handle());
                    }
                    Databases::EntityDb[eiidb].taskSequence.Reset();
                }
            }
        }
    }

    // Always show DB entity list with checkboxes
}

void Sub_PedOps()
{
    if (!selectedEntity.handle.Exists())
    {
        Menu::SetPreviousMenu();
        return;
    }
    selectedEntity.handle.RequestControlOnce();
    auto thisEntityIndexInDb = EntityManagement::GetEntityIndexInDb(selectedEntity);
    bool isThisEntityInDb = thisEntityIndexInDb >= 0;
    if (isThisEntityInDb)
    {
        selectedEntity.taskSequence = Databases::EntityDb[thisEntityIndexInDb].taskSequence;
        Databases::EntityDb[thisEntityIndexInDb] = selectedEntity;
    }

    GTAped myPed = PLAYER_PED_ID();
    GTAped thisPed = selectedEntity.handle;
    bool butAmIOnline = NETWORK_IS_IN_SESSION() != 0;
    bool isPedMyPed = thisPed.Handle() == myPed.Handle();
    bool bIsPedShortHeighted = GET_PED_CONFIG_FLAG(thisPed.Handle(), ePedConfigFlags::_Shrink, false) != 0;
    PedGroup myPedGroup = myPed.GetCurrentPedGroup();

    bool pedops_isStill_toggle = false, pedops_canRagdoll_toggle = false, pedops_shortHeighted_toggle = false, pedops_friend = false, pedops_burn = false, pedops_piggyback = false,
         pedops_shoulderRide = false, pedops_warp_nearest = false, pedops_wp_walk = false, pedops_become_ped = false;

    AddTitle(Language::Translate("menu.spooner.ped.title"));

    if (!isPedMyPed)
        MenuOptions::AddOption_RelationshipTextScroller();
    AddToggle(Language::Translate("menu.spooner.ped.option.still.label"), selectedEntity.isStill, pedops_isStill_toggle, pedops_isStill_toggle);
    AddOptionDescription(Language::Translate("menu.spooner.ped.option.still.description"));
    if (pedops_isStill_toggle)
    {
        thisPed.SetBlockPermanentEvent(selectedEntity.isStill);
    }
    AddLocal(Language::Translate("menu.spooner.ped.option.can_ragdoll.label"), thisPed.GetCanRagdoll(), pedops_canRagdoll_toggle, pedops_canRagdoll_toggle);
    if (pedops_canRagdoll_toggle)
    {
        bool ns = !thisPed.GetCanRagdoll();
        thisPed.SetCanRagdoll(ns);
        SET_PED_RAGDOLL_ON_COLLISION(thisPed.Handle(), ns);
    }
    AddLocal(Language::Translate("menu.spooner.ped.option.short_height.label"), bIsPedShortHeighted, pedops_shortHeighted_toggle, pedops_shortHeighted_toggle);
    AddOptionDescription(Language::Translate("menu.spooner.ped.option.short_height.description"));
    if (pedops_shortHeighted_toggle)
    {
        SET_PED_CONFIG_FLAG(selectedEntity.handle.Handle(), ePedConfigFlags::_Shrink, bIsPedShortHeighted ? 0 : 1);
    }

    int thisArmour = thisPed.GetArmour();
    bool bArmour_plus = false, bArmour_minus = false, bArmour_input = false;
    AddNumber(Language::Translate("menu.spooner.ped.option.armour.label"), thisArmour, 0, bArmour_input, bArmour_plus, bArmour_minus);
    if (bArmour_plus)
    {
        if (thisArmour < INT_MAX)
        {
            thisArmour++;
            thisPed.SetArmour(thisArmour);
        }
    }
    if (bArmour_minus)
    {
        if (thisArmour > 0)
        {
            thisArmour--;
            thisPed.SetArmour(thisArmour);
        }
    }
    if (bArmour_input)
    {
        std::string inputStr = Game::InputBox("", 5U, "", std::to_string(thisArmour));
        if (inputStr.length() > 0)
        {
            try
            {
                thisArmour = stoi(inputStr);
                thisPed.SetArmour(thisArmour);
            }
            catch (...)
            {
                Game::Print::PrintErrorInvalidInput(inputStr);
            }
        }
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SpoonerEoArmour, std::string(), 4U, std::string(), std::to_string(thisHealth));
        //OnscreenKeyboard::State::arg1._int = ent.GetHandle();
    }

    AddOption(Language::Translate("menu.spooner.ped.option.wardrobe.label"), null, SetSelectedEntityAsActivePed, SUB::PED_WARDROBE);

    AddOption(Language::Translate("menu.spooner.ped.option.animations.label"), null, SetSelectedEntityAsActivePed, SUB::ANIMATIONSUB);
    AddOption(Language::Translate("menu.spooner.ped.option.scenario_actions.label"), null, SetSelectedEntityAsActivePed, SUB::AnimationTaskScenarios);
    AddOption(Language::Translate("menu.spooner.ped.option.moods.label"), null, SetSelectedEntityAsActivePed, SUB::FACIALMOOD);
    AddOption(Language::Translate("menu.spooner.ped.option.movement_styles.label"), null, SetSelectedEntityAsActivePed, SUB::MOVEMENTGROUP);
    AddOption(Language::Translate("menu.spooner.ped.option.weapon.label"), null, nullFunc, SUB::SPOONER_PEDOPS_WEAPON);
    AddOption(Language::Translate("menu.spooner.ped.option.speech.label"), null, SetSelectedEntityAsActivePed, SUB::SPEECHPLAYER);
    AddOption(Language::Translate("menu.spooner.ped.option.voice_changer.label"), null, SetSelectedEntityAsActivePed, SUB::VOICECHANGER);
    //AddOption("Ped Flags (Doesn't Save)", null, SetSelectedEntityAsActivePed, SUB::PEDFLAGMANAGER_NAMEDLIST);
    //AddOption("Give Vehicle", null, SetSelectedEntityAsActivePed, SUB::SPAWNVEHICLE);
    AddLocal(Language::Translate("menu.spooner.ped.option.companion.label"), myPedGroup.Contains(thisPed), pedops_friend, pedops_friend);
    AddOptionDescription(Language::Translate("menu.spooner.ped.option.companion.description"));
    AddLocal(Language::Translate("menu.spooner.ped.option.burn.label"), thisPed.IsOnFire(), pedops_burn, pedops_burn);
    if (!isPedMyPed)
        AddLocal(
            Language::Translate("menu.spooner.ped.option.piggyback.label"),
            (GET_ENTITY_ATTACHED_TO(myPed.Handle() == thisPed.Handle()) && IS_ENTITY_PLAYING_ANIM(myPed.Handle(), "mini@prostitutes@sexnorm_veh", "bj_loop_male", 3)),
            pedops_piggyback,
            pedops_piggyback
        );
    if (!isPedMyPed)
        AddLocal(
            Language::Translate("menu.spooner.ped.option.shoulder_ride.label"),
            (GET_ENTITY_ATTACHED_TO(myPed.Handle() == thisPed.Handle()) && IS_ENTITY_PLAYING_ANIM(myPed.Handle(), "amb@prop_human_seat_chair@male@elbows_on_knees@idle_a", "idle_a", 3)),
            pedops_shoulderRide,
            pedops_shoulderRide
        );
    //AddOption("Warp Into Nearest Car", pedops_warp_nearest);
    AddOption(Language::Translate("menu.spooner.ped.option.travel_to_waypoint.label"), pedops_wp_walk);
    if (!isPedMyPed)
    {
        AddOption(Language::Translate("menu.spooner.ped.option.become.label"), butAmIOnline ? null : pedops_become_ped);
        AddOptionDescription(Language::Translate("menu.spooner.ped.option.become.description"));
    }


    if (pedops_friend)
    {
        NETWORK_REQUEST_CONTROL_OF_NETWORK_ID(thisPed.NetID());
        if (myPedGroup.Exists())
        {
            if (!myPedGroup.Contains(thisPed))
            {
                myPedGroup.Add(thisPed, false);
                myPedGroup.SetSeparationRange(100.0f);
                myPedGroup.SetFormationSpacing(1.5f);
                thisPed.Task().FightAgainstHatedTargets(400.0f);
            }
            else
            {
                myPedGroup.Remove(thisPed);
            }
        }
        else
        {
            myPedGroup = PedGroup::CreateNewGroup();
            myPedGroup.Add(myPed, true);
            myPedGroup.SetSeparationRange(100.0f);
            myPedGroup.SetFormationSpacing(1.5f);
        }
    }

    if (pedops_burn)
    {
        thisPed.SetOnFire(!thisPed.IsOnFire());
    }

    if (pedops_piggyback)
    {
        if (thisPed == myPed)
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.ped.cannot_target_self.message"));
        else
        {
            if (GET_ENTITY_ATTACHED_TO(myPed.Handle()) != thisPed.Handle())
            {
                thisPed.RequestControl();
                myPed.AttachTo(thisPed, -1, false, Vector3(0.0f, -0.3f, 0.0f), Vector3(0, 0, 0));
                myPed.Task().PlayAnimation("mini@prostitutes@sexnorm_veh", "bj_loop_male", 8.0f, 0.0f, -1, 9, 0, false);
            }
            else
            {
                myPed.Detach();
                myPed.Task().ClearAllImmediately();
            }
        }
    }

    if (pedops_shoulderRide)
    {
        if (thisPed == myPed)
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.ped.cannot_target_self.message"));
        else
        {
            if (GET_ENTITY_ATTACHED_TO(myPed.Handle()) != thisPed.Handle())
            {
                //set_ped_shoulder_ride_ped(myPed, thisPed, true);
                myPed.AttachTo(thisPed, Bone::SKEL_ROOT, Vector3(0, -0.25, 0.35f), Vector3(45.0f, 0, 0), true, true, false, true, 1, true);
                myPed.Task().PlayAnimation("amb@prop_human_seat_chair@male@elbows_on_knees@idle_a", "idle_a", 1000.0f, -1.5f, -1, AnimFlag::Loop, 0.445f, false);
            }
            else
            {
                //set_ped_shoulder_ride_ped(myPed, thisPed, false);
                myPed.Detach();
                myPed.Task().ClearAllImmediately();
            }
        }
    }

    if (pedops_warp_nearest)
    {
        const auto& closestVeh = World::GetClosestVehicle(thisPed.GetPosition(), 100.0f);
        if (closestVeh.Exists())
            thisPed.SetIntoVehicle(closestVeh, closestVeh.FirstFreeSeat(SEAT_DRIVER));
        else
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.ped.vehicle_not_found.message"));
    }

    if (pedops_wp_walk)
    {
        if (!IS_WAYPOINT_ACTIVE())
            Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.ped.no_waypoint.message"));
        else
        {
            Vector3 coord = GET_BLIP_INFO_ID_COORD(GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint));
            coord.z = World::GetGroundHeight(coord);
            //thisPed.Task().GoTo(coord);
            TASK_GO_TO_COORD_ANY_MEANS(thisPed.Handle(), coord.x, coord.y, coord.z, 3.0f, 0, 0, 786603, -1082130432);
        }
    }

    if (pedops_become_ped)
    {
        SetBecomePed(thisPed);
    }
}

void Sub_PedOps_Weapon()
{
    auto& selectedCategoryIndex = s_spoonerPedWeaponCategory;
    GTAped myPed = PLAYER_PED_ID();
    GTAped thisPed = selectedEntity.handle;
    Hash pedCurrWeapon = thisPed.GetWeapon();
    bool isPedMyPed = thisPed.Handle() == myPed.Handle();

    AddTitle(Language::Translate("menu.spooner.weapon.title"));

    bool bSetToNonePressed = false;
    AddTickol(Language::Translate("menu.spooner.weapon.option.none.label"), pedCurrWeapon == WEAPON_UNARMED, bSetToNonePressed, bSetToNonePressed);
    if (bSetToNonePressed)
    {
        pedCurrWeapon = WEAPON_UNARMED;
        SET_CURRENT_PED_WEAPON(thisPed.Handle(), pedCurrWeapon, true);
    }

    bool bMyCurrWeapon = false;
    if (!isPedMyPed)
        AddOption(Language::Translate("menu.spooner.weapon.option.current.label"), bMyCurrWeapon);
    if (bMyCurrWeapon)
    {
        //std::vector<s_Weapon_Components_Tint> weaponsBackup;
        //myPed.StoreWeaponsInArray(weaponsBackup);
        //GTAped(thisPed).GiveWeaponsFromArray(weaponsBackup);
        Hash weaponHash = myPed.GetWeapon();
        GIVE_DELAYED_WEAPON_TO_PED(thisPed.Handle(), weaponHash, 9999, true);
        GIVE_WEAPON_TO_PED(thisPed.Handle(), weaponHash, 1, true, false);
        int ammo;
        GET_MAX_AMMO(thisPed.Handle(), weaponHash, &ammo);
        SET_PED_AMMO(thisPed.Handle(), weaponHash, ammo, 0);
        SET_AMMO_IN_CLIP(thisPed.Handle(), weaponHash, GET_MAX_AMMO_IN_CLIP(thisPed.Handle(), weaponHash, true));
        SET_CURRENT_PED_WEAPON(thisPed.Handle(), weaponHash, true);
    }

    for (UINT i = 0; i < WeaponIndivs::vCategoryNames.size(); i++)
    {
        bool bCategoryPressed = false;
        AddOption(WeaponIndivs::vCategoryNames[i], bCategoryPressed, nullFunc, SUB::SPOONER_PEDOPS_WEAPON_INCATEGORY);
        if (bCategoryPressed)
        {
            selectedCategoryIndex = i;
        }
    }
}

void Sub_PedOps_Weapon_InCategory()
{
    auto& selectedCategoryIndex = s_spoonerPedWeaponCategory;
    AddTitle(WeaponIndivs::vCategoryNames[selectedCategoryIndex]);

    GTAped thisPed = selectedEntity.handle;
    Hash currentWeapon = thisPed.GetWeapon();

    for (auto& wc : *WeaponIndivs::vAllWeapons[selectedCategoryIndex])
    {
        bool bWeapPressed = false;
        AddTickol(GetWeaponLabel(wc.weaponHash, true), currentWeapon == wc.weaponHash, bWeapPressed, bWeapPressed);
        if (bWeapPressed)
        {
            GIVE_DELAYED_WEAPON_TO_PED(thisPed.Handle(), wc.weaponHash, 9999, true);
            GIVE_WEAPON_TO_PED(thisPed.Handle(), wc.weaponHash, 1, true, false);
            int ammo;
            GET_MAX_AMMO(thisPed.Handle(), wc.weaponHash, &ammo);
            SET_PED_AMMO(thisPed.Handle(), wc.weaponHash, ammo, 0);
            SET_AMMO_IN_CLIP(thisPed.Handle(), wc.weaponHash, GET_MAX_AMMO_IN_CLIP(thisPed.Handle(), wc.weaponHash, true));
            SET_CURRENT_PED_WEAPON(thisPed.Handle(), wc.weaponHash, true);
        }
    }
}

void Sub_ManageMarkers()
{
    int markerIndexInDbToDelete = -1;

    AddTitle(Language::Translate("menu.spooner.marker.title"));

    AddOption(Language::Translate("menu.spooner.marker.option.removal.label"), null, nullFunc, SUB::SPOONER_MANAGEMARKERS_REMOVAL);

    AddBreak(Language::Translate("menu.spooner.marker.section.database.label"));
    for (UINT i = 0; i < Databases::MarkerDb.size(); i++)
    {
        auto& m = Databases::MarkerDb[i];
        bool bMarkerPressed = false;

        AddOption(m.m_name, bMarkerPressed);
        if (bMarkerPressed)
        {
            SelectedMarker = &m;
            Menu::pendingSubmenu = SUB::SPOONER_MANAGEMARKERS_INMARKER;
        }


        if (Menu::IsLastDrawnOptionSelected())
        {
            m.m_selectedInSub = true;

            bool bShortcutDeletePressed;
            Keybinds::AddBindIB("menu_action", Language::Translate("menu.spooner.marker.option.delete.label"));
            bShortcutDeletePressed = Keybinds::WasPressedThisFrame("menu_action");

            if (bShortcutDeletePressed)
            {
                markerIndexInDbToDelete = i;
            }
        }
    }

    bool bAddNewMarkerPressed = false;
    AddTickol(Language::Translate("menu.spooner.marker.option.add_new.label"), true, bAddNewMarkerPressed, bAddNewMarkerPressed, TICKOL::SMALLNEWSTAR);
    if (bAddNewMarkerPressed)
    {
        auto& spoocam = SpoonerCamera::camera;
        if (!spoocam.IsActive())
        {
            GTAentity myPed = PLAYER_PED_ID();
            Vector3 myPos = myPed.GetPosition();
            SelectedMarker = MarkerManagement::AddMarker(myPos, Vector3(0, 0, myPed.GetHeading()));
        }
        else
        {
            Vector3 spawnPos = spoocam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 120.0f, 30.0f + SpoonerMarker().m_scale / 2);
            spawnPos.z += SpoonerMarker().m_scale / 2;
            SelectedMarker = MarkerManagement::AddMarker(spawnPos, Vector3(0, 0, spoocam.GetRotation().z));
        }
        Menu::pendingSubmenu = SUB::SPOONER_MANAGEMARKERS_INMARKER;
    }


    if (markerIndexInDbToDelete != -1)
    {
        MarkerManagement::RemoveMarker(markerIndexInDbToDelete);
    }

    if (Menu::IsSelectionPastDrawnOptions())
        Menu::Up(); // Go up if you're too far down due to removing a marker from DB from the properties menu and coming back here again.
}

void Sub_ManageMarkers_Removal()
{
    auto& fMarkerRemovalRadius = _fSaveRangeRadius;
    GTAentity myPed = PLAYER_PED_ID();
    const Vector3& myPos = myPed.GetPosition();

    AddTitle(Language::Translate("menu.spooner.marker.removal.title"));

    bool bInRange_plus = false, bInRange_minus = false, bInRange_execute = false;
    AddNumber(Language::Translate("menu.spooner.marker.option.delete_in_range.label"), fMarkerRemovalRadius, 0, bInRange_execute, bInRange_plus, bInRange_minus);
    AddOptionDescription(Language::Translate("menu.spooner.marker.option.delete_in_range.description"));
    if (Menu::IsLastDrawnOptionSelected())
        EntityManagement::DrawRadiusDisplayingMarker(myPos, fMarkerRemovalRadius);
    if (bInRange_plus)
    {
        if (fMarkerRemovalRadius < FLT_MAX)
            fMarkerRemovalRadius += 1.0f;
    }
    if (bInRange_minus)
    {
        if (fMarkerRemovalRadius > 0.0f)
            fMarkerRemovalRadius -= 1.0f;
    }
    if (bInRange_execute)
    {
        MarkerManagement::RemoveAllMarkersInRange(myPos, fMarkerRemovalRadius);
    }

    AddOption(Language::Format("menu.spooner.marker.option.delete_all.label", {{"count", Databases::MarkerDb.size()}}), null, MarkerManagement::RemoveAllMarkers);
}

void Sub_ManageMarkers_InMarker()
{
    if (SelectedMarker == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    AddTitle(SelectedMarker->m_name);

    Vector3 finalPosition, finalRotation;
    if (SelectedMarker->m_attachmentArgs.attachedTo.Exists())
    {
        finalPosition = SelectedMarker->m_attachmentArgs.attachedTo.GetOffsetInWorldCoords(SelectedMarker->m_attachmentArgs.offset);
        finalRotation = SelectedMarker->m_attachmentArgs.attachedTo.GetRotation() + SelectedMarker->m_attachmentArgs.rotation;
    }
    else
    {
        finalPosition = SelectedMarker->m_position;
        finalRotation = SelectedMarker->m_rotation;
    }

    SpoonerMarkerPosition* dest = SelectedMarker->m_destinationPtr == nullptr ? &SelectedMarker->m_destinationVal : SelectedMarker->m_destinationPtr;
    Vector3 finalDest;
    float finalDestHeading;
    if (dest->m_attachmentArgs.attachedTo.Exists())
    {
        finalDest = dest->m_attachmentArgs.attachedTo.GetOffsetInWorldCoords(dest->m_attachmentArgs.offset);
        finalDestHeading = dest->m_attachmentArgs.attachedTo.GetRotation().z + SelectedMarker->m_destinationHeading;
    }
    else
    {
        finalDest = dest->m_position;
        finalDestHeading = SelectedMarker->m_destinationHeading;
    }

    if (!dest->m_position.IsZero())
    {
        World::DrawLine(finalPosition, finalDest, RGBA(SelectedMarker->m_colour, 200));
        World::DrawLightWithRange(finalDest, RgbS(SelectedMarker->m_colour), 2.3f, 1.5f);

        const Vector3& helpingSpherePos = finalDest.PointOnCircle(1.16f, finalDestHeading + 90.0f);
        World::DrawLine(finalDest, helpingSpherePos, RGBA(SelectedMarker->m_colour, 200));
        World::DrawMarker(MarkerType::DebugSphere, helpingSpherePos, Vector3(), Vector3(), Vector3(0.1f, 0.1f, 0.1f), RGBA(SelectedMarker->m_colour, 200));
    }

    auto& spoocam = SpoonerCamera::camera;

    bool bEditNamePressed = false;
    AddTexter(Language::Translate("menu.spooner.marker.option.name.label"), 0, std::vector<std::string>{SelectedMarker->m_name}, bEditNamePressed);
    if (bEditNamePressed)
    {
        SelectedMarker->m_name = Game::InputBox(SelectedMarker->m_name, 26U, Language::Translate("menu.spooner.marker.option.name_prompt.prompt").c_str(), SelectedMarker->m_name);
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SetArg1String, SelectedMarker->m_name, 26U, "Enter custom marker name:", SelectedMarker->m_name);
        //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&SelectedMarker->m_name);
    }

    bool bType_plus = false, bType_minus = false;
    AddTexter(Language::Translate("menu.spooner.marker.option.type.label"), SelectedMarker->m_type, MarkerType::vNames, null, bType_plus, bType_minus);
    if (bType_plus)
    {
        if (SelectedMarker->m_type < MarkerType::vNames.size() - 1)
            SelectedMarker->m_type++;
    }
    if (bType_minus)
    {
        if (SelectedMarker->m_type > 0)
            SelectedMarker->m_type--;
    }

    bool bScale_plus = false, bScale_minus = false;
    AddNumber(Language::Translate("menu.spooner.marker.option.size.label"), SelectedMarker->m_scale, 2, null, bScale_plus, bScale_minus);
    if (bScale_plus)
    {
        if (SelectedMarker->m_scale < 10.0f)
            SelectedMarker->m_scale += 0.05f;
    }
    if (bScale_minus)
    {
        if (SelectedMarker->m_scale > 0.0f)
            SelectedMarker->m_scale -= 0.05f;
    }

    bool bShowNameToggle = false;
    AddTickol(Language::Translate("menu.spooner.marker.option.show_name.label"), SelectedMarker->m_showName, bShowNameToggle, bShowNameToggle, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.marker.option.show_name.description"));
    if (bShowNameToggle)
        SelectedMarker->m_showName = !SelectedMarker->m_showName;

    bool bRotateContinuouslyToggle = false;
    AddTickol(
        Language::Translate("menu.spooner.marker.option.rotate_continuously.label"),
        SelectedMarker->m_rotateContinuously,
        bRotateContinuouslyToggle,
        bRotateContinuouslyToggle,
        TICKOL::BOXTICK,
        TICKOL::BOXBLANK
    );
    if (bRotateContinuouslyToggle)
        SelectedMarker->m_rotateContinuously = !SelectedMarker->m_rotateContinuously;

    bool bAllowVehiclesToggle = false;
    AddTickol(Language::Translate("menu.spooner.marker.option.allow_vehicles.label"), SelectedMarker->m_allowVehicles, bAllowVehiclesToggle, bAllowVehiclesToggle, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.marker.option.allow_vehicles.description"));
    if (bAllowVehiclesToggle)
        SelectedMarker->m_allowVehicles = !SelectedMarker->m_allowVehicles;

    AddsettingscolOption(Language::Translate("menu.spooner.marker.option.colour.label"), SelectedMarker->m_colour);

    AddBreak(Language::Translate("menu.spooner.marker.section.position.label"));
    {
        AddOption("~italic~" + finalPosition.ToString(), null);

        if (!spoocam.IsActive())
        {
            bool bSetPosToMe = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_player_position.label"), bSetPosToMe);
            if (bSetPosToMe)
            {
                Vector3 myPos = GTAentity(PLAYER_PED_ID()).GetPosition();
                SelectedMarker->m_position = myPos;
                SelectedMarker->m_attachmentArgs.attachedTo = 0;
            }
        }
        else
        {
            bool bSetPosToHitCoords = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_camera_target.label"), bSetPosToHitCoords);
            AddOptionDescription(Language::Translate("menu.spooner.marker.option.set_to_camera_target.description"));
            if (bSetPosToHitCoords)
            {
                Vector3 hitCoords = spoocam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 160.0f, 3.0f);
                SelectedMarker->m_position = hitCoords;
                SelectedMarker->m_attachmentArgs.attachedTo = 0;
            }
        }
        if (IS_WAYPOINT_ACTIVE())
        {
            bool bSetPosToWp = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_waypoint.label"), bSetPosToWp);
            if (bSetPosToWp)
            {
                GTAblip wpBlip = GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint);
                Vector3 wpCoords = wpBlip.GetPosition();
                wpCoords.z = World::GetGroundHeight(wpCoords);
                SelectedMarker->m_position = wpCoords;
                SelectedMarker->m_attachmentArgs.attachedTo = 0;
            }
        }

        if (!Databases::EntityDb.empty())
        {
            AddOption(Language::Translate("menu.spooner.marker.option.attach_to_entity.label"), null, nullFunc, SUB::SPOONER_MANAGEMARKERS_INMARKER_ATTACH);
        }

        if (SelectedMarker->m_attachmentArgs.attachedTo.Exists())
        {
            bool bAdjustAttachmentForPosPressed = false;
            AddOption(Language::Translate("menu.spooner.marker.option.adjust_attachment.label"), bAdjustAttachmentForPosPressed, nullFunc, SUB::SPOONER_VECTOR3_MANUALEDITING);
            if (bAdjustAttachmentForPosPressed)
            {
                SpoonerVector3ManualEditingPtrs = std::make_tuple<GTAentity, Vector3*, Vector3*>(0, &SelectedMarker->m_attachmentArgs.offset, &SelectedMarker->m_attachmentArgs.rotation);
            }
        }
        else
        {
            bool bManualEditingForPosPressed = false;
            AddOption(Language::Translate("menu.spooner.option.editing.label"), bManualEditingForPosPressed, nullFunc, SUB::SPOONER_VECTOR3_MANUALEDITING);
            if (bManualEditingForPosPressed)
            {
                SpoonerVector3ManualEditingPtrs = std::make_tuple<GTAentity, Vector3*, Vector3*>(0, &SelectedMarker->m_position, &SelectedMarker->m_rotation);
            }
        }
    }

    AddBreak(Language::Translate("menu.spooner.marker.section.destination.label"));
    {
        AddOption("~italic~" + finalDest.ToString(), null);

        bool bSetPosToNull = false;
        AddTickol(Language::Translate("menu.spooner.marker.option.no_destination.label"), SelectedMarker->m_destinationVal.m_position.IsZero(), bSetPosToNull, bSetPosToNull);
        AddOptionDescription(Language::Translate("menu.spooner.marker.option.no_destination.description"));
        if (bSetPosToNull)
        {
            SelectedMarker->m_destinationVal.m_position.clear();
            SelectedMarker->m_destinationVal.m_attachmentArgs.attachedTo = 0;
            SelectedMarker->m_destinationPtr = nullptr;
        }

        if (!spoocam.IsActive())
        {
            bool bSetPosToMe = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_player_position.label"), bSetPosToMe);
            if (bSetPosToMe)
            {
                Vector3 myPos = GTAentity(PLAYER_PED_ID()).GetPosition();
                SelectedMarker->m_destinationVal.m_position = myPos;
                SelectedMarker->m_destinationVal.m_attachmentArgs.attachedTo = 0;
                SelectedMarker->m_destinationPtr = nullptr;
            }
        }
        else
        {
            bool bSetPosToHitCoords = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_camera_target.label"), bSetPosToHitCoords);
            AddOptionDescription(Language::Translate("menu.spooner.marker.option.set_to_camera_target.description"));
            if (bSetPosToHitCoords)
            {
                Vector3 hitCoords = spoocam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 160.0f, 3.0f);
                SelectedMarker->m_destinationVal.m_position = hitCoords;
                SelectedMarker->m_destinationVal.m_attachmentArgs.attachedTo = 0;
                SelectedMarker->m_destinationPtr = nullptr;
            }
        }
        if (IS_WAYPOINT_ACTIVE())
        {
            bool bSetPosToWp = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_waypoint.label"), bSetPosToWp);
            if (bSetPosToWp)
            {
                GTAblip wpBlip = GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint);
                Vector3 wpCoords = wpBlip.GetPosition();
                wpCoords.z = World::GetGroundHeight(wpCoords);
                SelectedMarker->m_destinationVal.m_position = wpCoords;
                SelectedMarker->m_destinationVal.m_attachmentArgs.attachedTo = 0;
                SelectedMarker->m_destinationPtr = nullptr;
            }
        }

        if (Databases::MarkerDb.size() > 1)
        {
            AddOption(Language::Translate("menu.spooner.marker.option.link_with_marker.label"), null, nullFunc, SUB::SPOONER_MANAGEMARKERS_INMARKER_DEST2MARKER);
            if (SelectedMarker->m_destinationPtr != nullptr)
                AddOptionDescription(Language::Format("menu.spooner.markers.option.link_with_marker.description", {{"name", SelectedMarker->m_destinationPtr->m_name}}));
            else
                AddOptionDescription(Language::Translate("menu.spooner.marker.option.link_with_marker.description"));
        }

        if (SelectedMarker->m_destinationPtr == nullptr)
        {
            if (SelectedMarker->m_destinationVal.m_attachmentArgs.attachedTo.Exists())
            {
                bool bAdjustAttachmentForPosPressed = false;
                AddOption(Language::Translate("menu.spooner.marker.option.adjust_attachment.label"), bAdjustAttachmentForPosPressed, nullFunc, SUB::SPOONER_VECTOR3_MANUALEDITING);
                if (bAdjustAttachmentForPosPressed)
                {
                    SpoonerVector3ManualEditingPtrs = std::make_tuple<GTAentity, Vector3*, Vector3*>(0, &SelectedMarker->m_destinationVal.m_attachmentArgs.offset, nullptr);
                }
            }
            else
            {
                bool bManualEditingForPosPressed = false;
                AddOption(Language::Translate("menu.spooner.option.editing.label"), bManualEditingForPosPressed, nullFunc, SUB::SPOONER_VECTOR3_MANUALEDITING);
                if (bManualEditingForPosPressed)
                {
                    SpoonerVector3ManualEditingPtrs = std::make_tuple<GTAentity, Vector3*, Vector3*>(0, &SelectedMarker->m_destinationVal.m_position, nullptr);
                }
            }
        }

        bool bDestHeading_plus = false, bDestHeading_minus = false;
        AddNumber(Language::Translate("menu.spooner.marker.option.destination_heading.label"), SelectedMarker->m_destinationHeading, 1, null, bDestHeading_plus, bDestHeading_minus);
        AddOptionDescription(Language::Translate("menu.spooner.marker.option.destination_heading.description"));
        if (bDestHeading_plus)
        {
            SelectedMarker->m_destinationHeading += 1.0f;
        }
        if (bDestHeading_minus)
        {
            SelectedMarker->m_destinationHeading -= 1.0f;
        }
    }

    AddBreak(Language::Translate("menu.spooner.marker.section.other.label"));

    bool bCopyMarkerPressed = false;
    AddOption(Language::Translate("menu.spooner.marker.option.copy.label"), bCopyMarkerPressed);
    if (bCopyMarkerPressed)
    {
        SelectedMarker = MarkerManagement::CopyMarker(*SelectedMarker); // std::vector array location may change
        SelectedMarker->m_name = Game::InputBox(SelectedMarker->m_name, 26U, Language::Translate("menu.spooner.marker.option.name_prompt.prompt").c_str(), SelectedMarker->m_name);
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SetArg1String, SelectedMarker->m_name, 26U, "Enter custom marker name:", SelectedMarker->m_name);
        //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&SelectedMarker->m_name);
        Menu::optionSelectionHistory[Menu::menuHistoryIndex]++;
    }
}

void Sub_ManageMarkers_InMarker_Dest2Marker()
{
    if (SelectedMarker == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    AddTitle(Language::Translate("menu.spooner.marker.select.title"));

    for (UINT i = 0; i < Databases::MarkerDb.size(); i++)
    {
        auto& m = Databases::MarkerDb[i];
        if (&m != SelectedMarker)
        {
            bool bMarkerPressed = false;
            AddOption(m.m_name, bMarkerPressed);
            if (bMarkerPressed)
            {
                SelectedMarker->m_destinationPtr = &m;
                SelectedMarker->m_destinationVal = m;
                Menu::SetPreviousMenu();
                return;
            }

            if (Menu::IsLastDrawnOptionSelected())
            {
                m.m_selectedInSub = true;

                Vector3 finalPosition, finalRotation;
                if (SelectedMarker->m_attachmentArgs.attachedTo.Exists())
                {
                    finalPosition = SelectedMarker->m_attachmentArgs.attachedTo.GetOffsetInWorldCoords(SelectedMarker->m_attachmentArgs.offset);
                    finalRotation = SelectedMarker->m_attachmentArgs.attachedTo.GetRotation() + SelectedMarker->m_attachmentArgs.rotation;
                }
                else
                {
                    finalPosition = SelectedMarker->m_position;
                    finalRotation = SelectedMarker->m_rotation;
                }

                SpoonerMarkerPosition* dest = &m;
                Vector3 finalDest;
                float finalDestHeading;
                if (dest->m_attachmentArgs.attachedTo.Exists())
                {
                    finalDest = dest->m_attachmentArgs.attachedTo.GetOffsetInWorldCoords(dest->m_attachmentArgs.offset);
                    finalDestHeading = dest->m_attachmentArgs.attachedTo.GetRotation().z + SelectedMarker->m_destinationHeading;
                }
                else
                {
                    finalDest = dest->m_position;
                    finalDestHeading = SelectedMarker->m_destinationHeading;
                }

                World::DrawLine(finalPosition, finalDest, RGBA(SelectedMarker->m_colour, 200));
                World::DrawLightWithRange(finalDest, RgbS(SelectedMarker->m_colour), 2.3f, 1.5f);
            }
        }
    }
}

void Sub_ManageMarkers_InMarker_Attach()
{
    if (SelectedMarker == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    AddTitle(Language::Translate("menu.spooner.attachment.attach_to.title"));

    bool bToggleKeepPosWhenAttaching = false;
    AddTickol(
        Language::Translate("menu.spooner.attachment.option.keep_world_position.label"),
        Settings::bKeepPositionWhenAttaching,
        bToggleKeepPosWhenAttaching,
        bToggleKeepPosWhenAttaching,
        TICKOL::BOXTICK,
        TICKOL::BOXBLANK
    );
    AddOptionDescription(Language::Translate("menu.spooner.attachment.option.keep_world_position.description"));
    if (bToggleKeepPosWhenAttaching)
        Settings::bKeepPositionWhenAttaching = !Settings::bKeepPositionWhenAttaching;

    bool bDetachPressed = false;
    AddTickol(Language::Translate("menu.spooner.attachment.option.detach.label"), !SelectedMarker->m_attachmentArgs.attachedTo.GetHandle(), bDetachPressed, bDetachPressed, TICKOL::TICK2);
    if (bDetachPressed)
    {
        if (SelectedMarker->m_attachmentArgs.attachedTo.Exists())
        {
            SelectedMarker->m_position = SelectedMarker->m_attachmentArgs.attachedTo.GetOffsetInWorldCoords(SelectedMarker->m_attachmentArgs.offset);
            SelectedMarker->m_rotation = SelectedMarker->m_attachmentArgs.attachedTo.GetRotation() + SelectedMarker->m_attachmentArgs.rotation;
        }
        SelectedMarker->m_attachmentArgs.attachedTo = 0;
        SelectedMarker->m_attachmentArgs.offset.clear();
        SelectedMarker->m_attachmentArgs.rotation.clear();
    }

    if (!Databases::EntityDb.empty())
    {
        AddBreak(Language::Translate("menu.spooner.attachment.section.database.label"));
        for (auto& e : Databases::EntityDb)
        {
            if (e.handle.Exists())
            {
                bool bEntityPressed = false;
                AddTickol(e.hashName, SelectedMarker->m_attachmentArgs.attachedTo == e.handle, bEntityPressed, bEntityPressed, TICKOL::TICK2);
                if (bEntityPressed)
                {
                    SelectedMarker->m_attachmentArgs.attachedTo = e.handle;
                    if (Settings::bKeepPositionWhenAttaching)
                    {
                        SelectedMarker->m_attachmentArgs.offset = e.handle.GetOffsetGivenWorldCoords(SelectedMarker->m_position);
                        SelectedMarker->m_attachmentArgs.rotation = SelectedMarker->m_rotation - e.handle.GetRotation();
                    }
                    else
                    {
                        SelectedMarker->m_attachmentArgs.offset.clear();
                        SelectedMarker->m_attachmentArgs.rotation.clear();
                    }
                    Menu::SetPreviousMenu();
                    return;
                }

                if (Menu::IsLastDrawnOptionSelected())
                    EntityManagement::ShowArrowAboveEntity(e.handle, RGBA(0, 255, 0, 200));
            }
            else
            {
                AddOption(e.hashName + " (Invalid)", null);
            }
        }
    }
}

void Sub_ManageLights()
{
    int lightIndexInDbToDelete = -1;

    LightManagement::DrawPreviewMarkers();

    AddTitle(Language::Translate("menu.spooner.light.title"));

    AddOption(Language::Translate("menu.spooner.light.option.removal.label"), null, nullFunc, SUB::SPOONER_MANAGELIGHTS_REMOVAL);
    AddOption(Language::Translate("menu.spooner.light.option.manage_presets.label"), null, nullFunc, SUB::SPOONER_MANAGELIGHTS_PRESETS);

    AddBreak(Language::Translate("menu.spooner.light.section.database.label"));
    for (UINT i = 0; i < Databases::LightDb.size(); i++)
    {
        auto& l = Databases::LightDb[i];
        bool bLightPressed = false;

        AddOption(l.m_name, bLightPressed);
        if (bLightPressed)
        {
            SelectedLight = &l;
            Menu::pendingSubmenu = SUB::SPOONER_MANAGELIGHTS_INLIGHT;
        }

        if (Menu::IsLastDrawnOptionSelected())
        {
            l.m_selectedInSub = true;

            bool bShortcutDeletePressed;
            Keybinds::AddBindIB("menu_action", Language::Translate("menu.spooner.light.option.delete.label"));
            bShortcutDeletePressed = Keybinds::WasPressedThisFrame("menu_action");

            if (bShortcutDeletePressed)
            {
                lightIndexInDbToDelete = i;
            }
        }
    }

    bool bAddNewLightPressed = false;
    AddTickol(Language::Translate("menu.spooner.light.option.add_new.label"), true, bAddNewLightPressed, bAddNewLightPressed, TICKOL::SMALLNEWSTAR);
    if (bAddNewLightPressed)
    {
        auto& spoonerCam = SpoonerCamera::camera;
        if (!spoonerCam.IsActive())
        {
            GTAentity myPed = PLAYER_PED_ID();
            Vector3 pos = myPed.GetPosition() + myPed.ForwardVector() * 3.0f;
            pos.z += 1.0f;
            Vector3 dir = myPed.ForwardVector();
            SelectedLight = LightManagement::Add(SpoonerLight(pos, dir));
        }
        else
        {
            Vector3 pos = spoonerCam.GetPosition();
            Vector3 target = spoonerCam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 120.0f, 30.0f);
            Vector3 dir = Vector3::Normalize(target - pos);
            SelectedLight = LightManagement::Add(SpoonerLight(pos, dir));
        }
        Menu::pendingSubmenu = SUB::SPOONER_MANAGELIGHTS_INLIGHT;
    }

    if (!LightManagement::PresetDb.empty())
    {
        AddBreak(Language::Translate("menu.spooner.light.section.presets.label"));
        for (auto& p : LightManagement::PresetDb)
        {
            bool bPresetPressed = false;
            std::string presetLabel = (p.m_lightType == SpoonerLight::LightType::Omni ? "[Omni] " : "[Spot] ") + p.m_name;
            AddOption(presetLabel, bPresetPressed);
            if (bPresetPressed)
            {
                SpoonerLight copy = p;
                auto& spoonerCam = SpoonerCamera::camera;
                if (spoonerCam.IsActive())
                {
                    copy.m_position = spoonerCam.GetPosition();
                    Vector3 target = spoonerCam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 120.0f, 30.0f);
                    copy.m_direction = Vector3::Normalize(target - copy.m_position);
                }
                else
                {
                    GTAped myPed = PLAYER_PED_ID();
                    copy.m_position = myPed.GetPosition() + myPed.ForwardVector() * 3.0f;
                    copy.m_direction = myPed.ForwardVector();
                }
                SelectedLight = LightManagement::Add(copy);
                Menu::pendingSubmenu = SUB::SPOONER_MANAGELIGHTS_INLIGHT;
            }
        }
    }

    if (lightIndexInDbToDelete != -1)
    {
        LightManagement::Remove(lightIndexInDbToDelete);
    }

    if (Menu::IsSelectionPastDrawnOptions())
        Menu::Up();
}

void Sub_ManageLights_Removal()
{
    LightManagement::DrawPreviewMarkers();

    auto& fLightRemovalRadius = _fSaveRangeRadius;
    GTAentity myPed = PLAYER_PED_ID();
    const Vector3& myPos = myPed.GetPosition();

    AddTitle(Language::Translate("menu.spooner.light.removal.title"));

    bool bInRange_plus = false, bInRange_minus = false, bInRange_execute = false;
    AddNumber(Language::Translate("menu.spooner.light.option.delete_in_range.label"), fLightRemovalRadius, 0, bInRange_execute, bInRange_plus, bInRange_minus);
    AddOptionDescription(Language::Translate("menu.spooner.light.option.delete_in_range.description"));
    if (Menu::IsLastDrawnOptionSelected())
        EntityManagement::DrawRadiusDisplayingMarker(myPos, fLightRemovalRadius);
    if (bInRange_plus)
    {
        if (fLightRemovalRadius < FLT_MAX)
            fLightRemovalRadius += 1.0f;
    }
    if (bInRange_minus)
    {
        if (fLightRemovalRadius > 0.0f)
            fLightRemovalRadius -= 1.0f;
    }
    if (bInRange_execute)
    {
        for (int i = static_cast<int>(Databases::LightDb.size()) - 1; i >= 0; i--)
        {
            if (myPos.DistanceTo(Databases::LightDb[i].m_position) <= fLightRemovalRadius)
                LightManagement::Remove(i);
        }
    }

    AddOption(Language::Format("menu.spooner.light.option.delete_all.label", {{"count", Databases::LightDb.size()}}), null, LightManagement::RemoveAll);
}

void Sub_ManageLights_InLight()
{
    if (SelectedLight == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    LightManagement::DrawPreviewMarkers();

    auto& spoonerCam = SpoonerCamera::camera;

    AddTitle(SelectedLight->m_name);

    bool bEditNamePressed = false;
    AddTexter(Language::Translate("menu.spooner.light.option.name.label"), 0, std::vector<std::string>{SelectedLight->m_name}, bEditNamePressed);
    if (bEditNamePressed)
    {
        SelectedLight->m_name = Game::InputBox(SelectedLight->m_name, 26U, Language::Translate("menu.spooner.light.option.name_prompt.prompt").c_str(), SelectedLight->m_name);
    }

    SelectedLight->m_lightType = static_cast<SpoonerLight::LightType>(AddTexterCycler(
        Language::Translate("menu.spooner.light.option.type.label"),
        static_cast<int>(SelectedLight->m_lightType),
        {Language::Translate("menu.spooner.option.light.value.omnidirectional"), Language::Translate("menu.spooner.option.light.value.spot")}
    ));

    bool bActiveToggle = false;
    AddTickol(Language::Translate("menu.spooner.light.option.active.label"), SelectedLight->m_active, bActiveToggle, bActiveToggle, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    if (bActiveToggle)
        SelectedLight->m_active = !SelectedLight->m_active;

    AddBreak(Language::Translate("menu.spooner.light.section.colour.label"));
    {
        bool bColourPressed = false;
        AddOption(Language::Translate("menu.spooner.light.option.colour.label"), bColourPressed, nullFunc, SUB::SPOONER_MANAGELIGHTS_COLOUR);
    }

    if (SelectedLight->m_lightType == SpoonerLight::LightType::Omni)
    {
        AddBreak(Language::Translate("menu.spooner.light.section.omni_properties.label"));

        AddNumberStepper(Language::Translate("menu.spooner.light.option.range.label"), SelectedLight->m_range, 1, 0.5, 0.0, 1000.0);
        AddOptionDescription(Language::Translate("menu.spooner.light.option.range.description"));
        AddNumberStepper(Language::Translate("menu.spooner.light.option.intensity.label"), SelectedLight->m_intensity, 2, 0.1, 0.0, 100.0);
    }
    else
    {
        AddBreak(Language::Translate("menu.spooner.light.section.spot_properties.label"));

        AddNumberStepper(Language::Translate("menu.spooner.light.option.distance.label"), SelectedLight->m_spotDistance, 1, 0.5, 0.0, 1000.0);
        AddOptionDescription(Language::Translate("menu.spooner.light.option.distance.description"));
        AddNumberStepper(Language::Translate("menu.spooner.light.option.brightness.label"), SelectedLight->m_spotBrightness, 2, 0.1, 0.0, 100.0);
        AddNumberStepper(Language::Translate("menu.spooner.light.option.roundness.label"), SelectedLight->m_spotRoundness, 2, 0.1, 0.0, 10.0);
        AddOptionDescription(Language::Translate("menu.spooner.light.option.roundness.description"));
        AddNumberStepper(Language::Translate("menu.spooner.light.option.radius.label"), SelectedLight->m_spotRadius, 2, 0.1, 0.0, 100.0);
        AddOptionDescription(Language::Translate("menu.spooner.light.option.radius.description"));
        AddNumberStepper(Language::Translate("menu.spooner.light.option.falloff.label"), SelectedLight->m_spotFalloff, 2, 0.1, 0.0, 100.0);
        AddOptionDescription(Language::Translate("menu.spooner.light.option.falloff.description"));

        bool bShadowToggle = false;
        AddTickol(Language::Translate("menu.spooner.light.option.shadows.label"), SelectedLight->m_useShadow, bShadowToggle, bShadowToggle, TICKOL::BOXTICK, TICKOL::BOXBLANK);
        AddOptionDescription(Language::Translate("menu.spooner.light.option.shadows.description"));
        if (bShadowToggle)
            SelectedLight->m_useShadow = !SelectedLight->m_useShadow;
    }

    AddBreak(Language::Translate("menu.spooner.light.section.position.label"));
    {
        AddOption("~italic~" + SelectedLight->m_position.ToString(), null);

        if (!spoonerCam.IsActive())
        {
            bool bSetPosToMe = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_player_position.label"), bSetPosToMe);
            if (bSetPosToMe)
            {
                SelectedLight->m_position = GTAentity(PLAYER_PED_ID()).GetPosition();
            }
        }
        else
        {
            bool bSetPosToHitCoords = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_camera_target.label"), bSetPosToHitCoords);
            AddOptionDescription(Language::Translate("menu.spooner.marker.option.set_to_camera_target.description"));
            if (bSetPosToHitCoords)
            {
                Vector3 hitCoords = spoonerCam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 160.0f, 3.0f);
                SelectedLight->m_position = hitCoords;
            }
        }
        if (spoonerCam.IsActive())
        {
            bool bSetPosToCam = false;
            AddOption(Language::Translate("menu.spooner.light.option.set_to_camera_position.label"), bSetPosToCam);
            AddOptionDescription(Language::Translate("menu.spooner.light.option.set_to_camera_position.description"));
            if (bSetPosToCam)
            {
                SelectedLight->m_position = spoonerCam.GetPosition();
            }
        }

        if (IS_WAYPOINT_ACTIVE())
        {
            bool bSetPosToWp = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_waypoint.label"), bSetPosToWp);
            if (bSetPosToWp)
            {
                GTAblip wpBlip = GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint);
                Vector3 wpCoords = wpBlip.GetPosition();
                wpCoords.z = World::GetGroundHeight(wpCoords);
                SelectedLight->m_position = wpCoords;
            }
        }

        bool bManualEditingForPosPressed = false;
        AddOption(Language::Translate("menu.spooner.option.editing.label"), bManualEditingForPosPressed, nullFunc, SUB::SPOONER_VECTOR3_MANUALEDITING);
        if (bManualEditingForPosPressed)
        {
            SpoonerVector3ManualEditingPtrs = std::make_tuple<GTAentity, Vector3*, Vector3*>(0, &SelectedLight->m_position, nullptr);
        }
    }

    if (SelectedLight->m_lightType == SpoonerLight::LightType::Spot)
    {
        AddBreak(Language::Translate("menu.spooner.light.section.direction.label"));
        {
            AddOption("~italic~" + SelectedLight->m_direction.ToString(), null);

            if (spoonerCam.IsActive())
            {
                bool bPointAtCursor = false;
                AddOption(Language::Translate("menu.spooner.light.option.point_at_cursor.label"), bPointAtCursor);
                AddOptionDescription(Language::Translate("menu.spooner.light.option.point_at_cursor.description"));
                if (bPointAtCursor)
                {
                    Vector3 target = spoonerCam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 160.0f, 3.0f);
                    Vector3 dir = Vector3::Normalize(target - SelectedLight->m_position);
                    SelectedLight->m_direction = dir;
                }
            }

            bool bManualEditingForDirPressed = false;
            AddOption(Language::Translate("menu.spooner.option.editing.label"), bManualEditingForDirPressed, nullFunc, SUB::SPOONER_VECTOR3_MANUALEDITING);
            if (bManualEditingForDirPressed)
            {
                SpoonerVector3ManualEditingPtrs = std::make_tuple<GTAentity, Vector3*, Vector3*>(0, &SelectedLight->m_direction, nullptr);
            }
        }
    }

    AddBreak(Language::Translate("menu.spooner.light.section.other.label"));

    bool bCopyLightPressed = false;
    AddOption(Language::Translate("menu.spooner.light.option.copy.label"), bCopyLightPressed);
    if (bCopyLightPressed)
    {
        SelectedLight = LightManagement::Copy(*SelectedLight);
        Menu::optionSelectionHistory[Menu::menuHistoryIndex]++;
    }

    bool bSavePresetPressed = false;
    AddOption(Language::Translate("menu.spooner.light.option.save_as_preset.label"), bSavePresetPressed);
    if (bSavePresetPressed)
    {
        std::string presetName = Game::InputBox(SelectedLight->m_name, 26U, Language::Translate("menu.spooner.light.option.preset_name_prompt.prompt").c_str(), SelectedLight->m_name);
        if (presetName.length() > 0)
        {
            SpoonerLight copy = *SelectedLight;
            copy.m_name = presetName;
            LightManagement::SavePresetToFile(copy);
        }
    }
}

void Sub_ManageLights_Presets()
{
    AddTitle(Language::Translate("menu.spooner.light.presets.title"));

    if (SelectedLight != nullptr)
    {
        bool bSaveCurrentPressed = false;
        AddOption(Language::Translate("menu.spooner.light.option.save_current_as_preset.label"), bSaveCurrentPressed);
        if (bSaveCurrentPressed)
        {
            std::string presetName = Game::InputBox(SelectedLight->m_name, 26U, Language::Translate("menu.spooner.light.option.preset_name_prompt.prompt").c_str(), SelectedLight->m_name);
            if (presetName.length() > 0)
            {
                SpoonerLight copy = *SelectedLight;
                copy.m_name = presetName;
                LightManagement::SavePresetToFile(copy);
                LightManagement::LoadPresetsFromFile(GetPathffA(Pathff::RootDir, true) + "FavouriteLights.xml");
            }
        }
    }

    LightManagement::LoadPresetsFromFile(GetPathffA(Pathff::RootDir, true) + "FavouriteLights.xml");

    if (!LightManagement::PresetDb.empty())
    {
        AddBreak(Language::Translate("menu.spooner.light.section.saved_presets.label"));
        for (size_t i = 0; i < LightManagement::PresetDb.size(); i++)
        {
            auto& p = LightManagement::PresetDb[i];
            bool bPresetPressed = false;
            std::string presetLabel = (p.m_lightType == SpoonerLight::LightType::Omni ? "[Omni] " : "[Spot] ") + p.m_name;
            AddOption(presetLabel, bPresetPressed);
            if (bPresetPressed)
            {
                SpoonerLight copy = p;
                auto& spoonerCam = SpoonerCamera::camera;
                if (spoonerCam.IsActive())
                {
                    copy.m_position = spoonerCam.GetPosition();
                    Vector3 target = spoonerCam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 120.0f, 30.0f);
                    copy.m_direction = Vector3::Normalize(target - copy.m_position);
                }
                else
                {
                    GTAped myPed = PLAYER_PED_ID();
                    copy.m_position = myPed.GetPosition() + myPed.ForwardVector() * 3.0f;
                    copy.m_direction = myPed.ForwardVector();
                }
                LightManagement::Add(copy);
                Game::Print::ShowNotification(Language::Translate("message.spooner.light.added_from_preset.title"));
            }

            if (Menu::IsLastDrawnOptionSelected())
            {
                bool bDeletePressed;
                Keybinds::AddBindIB("menu_action", Language::Translate("menu.spooner.light.option.delete_preset.label"));
                bDeletePressed = Keybinds::WasPressedThisFrame("menu_action");
                if (bDeletePressed)
                {
                    LightManagement::PresetDb.erase(LightManagement::PresetDb.begin() + i);
                    LightManagement::SaveAllPresetsToFile();
                    Menu::optionSelectionHistory[Menu::menuHistoryIndex]--;
                    break;
                }
            }
        }
    }
    else
    {
        AddOption(Language::Translate("menu.spooner.light.option.no_presets.label"), null);
    }
}

void Sub_ManageLights_Colour()
{
    if (SelectedLight == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    LightManagement::DrawPreviewMarkers();

    RGBA* colour = &SelectedLight->m_colour;

    AddTitle(Language::Translate("menu.spooner.light.colour_title.title"));
    AddNumberStepper(Language::Translate("menu.spooner.light.option.red.label"), colour->R, 0, 1.0, 0, 255);
    AddNumberStepper(Language::Translate("menu.spooner.light.option.green.label"), colour->G, 0, 1.0, 0, 255);
    AddNumberStepper(Language::Translate("menu.spooner.light.option.blue.label"), colour->B, 0, 1.0, 0, 255);

    switch (*Menu::activeOptionIndex)
    {
    case 1:
    case 2:
    case 3:
        AddPresetColourOptionsPreviews(colour->R, colour->G, colour->B);
        break;
    }

    {
        bool bHexInputPressed = false;
        AddOption(Language::Translate("menu.spooner.light.option.input_hex.label"), bHexInputPressed);
        if (bHexInputPressed)
        {
            std::string input = Game::InputBox("", 10U, Language::Translate("menu.spooner.light.option.hex_prompt.prompt").c_str(), "#");
            if (!HexToRGBA(input, *colour))
                Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.light.invalid_hex.message"));
        }
    }
}

void Sub_Blip_Management()
{
    int blipIndexInDbToDelete = -1;
    AddTitle(Language::Translate("menu.spooner.blip.management.title"));

    //AddOption("Add Blip", null, nullFunc, SUB::SPOONER_BLIPS_ADD_SELECT);

    bool bAttachBlipToEntityPressed = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.create_entity.label"), true, bAttachBlipToEntityPressed, bAttachBlipToEntityPressed, TICKOL::SMALLNEWSTAR);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.create_entity.description"));
    if (bAttachBlipToEntityPressed)
    {
        Menu::pendingSubmenu = SUB::SPOONER_BLIPS_ENTITY_SELECT;
    }

    bool bAddNewCoordBlipPressed = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.create_coord.label"), true, bAddNewCoordBlipPressed, bAddNewCoordBlipPressed, TICKOL::SMALLNEWSTAR);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.create_coord.description"));
    if (bAddNewCoordBlipPressed)
    {
        auto& spoocam = SpoonerCamera::camera;

        if (!spoocam.IsActive())
        {
            GTAentity myPed = PLAYER_PED_ID();
            Vector3 myPos = myPed.GetPosition();

            sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(myPos, Vector3(0, 0, myPed.GetHeading()));
        }
        else
        {
            Vector3 spawnPos = spoocam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 120.0f, 30.0f);

            sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(spawnPos, Vector3(0, 0, spoocam.GetRotation().z));
        }

        sub::Spooner::SelectedBlip->BlipType = SpoonerBlip::Type::Coord;
        sub::Spooner::SelectedBlip->Alpha = 255;
        sub::Spooner::SelectedBlip->Scale = 0.80f;
        BlipCustoms::RefreshBlip(*sub::Spooner::SelectedBlip);
        Menu::pendingSubmenu = SUB::SPOONER_BLIPS_COORDINBLIP;
    }

    bool bAddNewRadialBlipPressed = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.create_radial.label"), true, bAddNewRadialBlipPressed, bAddNewRadialBlipPressed, TICKOL::SMALLNEWSTAR);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.create_radial.description"));
    if (bAddNewRadialBlipPressed)
    {
        auto& spoocam = SpoonerCamera::camera;

        if (!spoocam.IsActive())
        {
            GTAentity myPed = PLAYER_PED_ID();
            Vector3 myPos = myPed.GetPosition();

            sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(myPos, Vector3(0, 0, myPed.GetHeading()));
        }
        else
        {
            Vector3 spawnPos = spoocam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 120.0f, 30.0f);

            sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(spawnPos, Vector3(0, 0, spoocam.GetRotation().z));
        }

        sub::Spooner::SelectedBlip->BlipType = SpoonerBlip::Type::Radial;
        sub::Spooner::SelectedBlip->Alpha = 160;
        sub::Spooner::SelectedBlip->Scale = 0.80f;
        BlipCustoms::RefreshBlip(*sub::Spooner::SelectedBlip);
        Menu::pendingSubmenu = SUB::SPOONER_BLIPS_RADIALINBLIP;
    }

    AddBreak(Language::Translate("menu.spooner.blip.section.entity_section.label"));
    bool bHasEntity = false;
    for (auto& b : Databases::BlipDb)
        if (b.BlipType == SpoonerBlip::Type::Entity)
        {
            bHasEntity = true;
            break;
        }
    if (!bHasEntity)
    {
        bool bAddEntity = false;
        AddOption(Language::Translate("menu.spooner.blip.option.add_entity.label"), bAddEntity);
        if (bAddEntity)
            Menu::pendingSubmenu = SUB::SPOONER_BLIPS_ENTITY_SELECT;
    }
    for (UINT i = 0; i < Databases::BlipDb.size(); i++)
    {
        auto& m = Databases::BlipDb[i];
        if (m.BlipType != SpoonerBlip::Type::Entity)
            continue;

        bool bBlipPressed = false;
        std::string displayName = m.label.empty() ? m.Name : m.label;
        AddOption(displayName, bBlipPressed);
        if (bBlipPressed)
        {
            sub::Spooner::SelectedBlip = &m;
            Menu::pendingSubmenu = SUB::SPOONER_BLIPS_RADIALINBLIP;
        }


        if (*Menu::activeOptionIndex == Menu::currentOptionCount)
        {
            m.m_selectedInSub = true;

            GTAentity ent = m.EntityHandle;
            EntityManagement::ShowArrowAboveEntity(ent, RGBA(0, 255, 255, 200));

            bool bShortcutDeletePressed;
            Keybinds::AddBindIB("menu_action", Language::Translate("menu.spooner.blip.option.delete_entity.label"));
            bShortcutDeletePressed = Keybinds::WasPressedThisFrame("menu_action");

            if (bShortcutDeletePressed)
                blipIndexInDbToDelete = i;
        }
    }

    AddBreak(Language::Translate("menu.spooner.blip.section.coord_section.label"));
    bool bHasCoord = false;
    for (auto& b : Databases::BlipDb)
        if (b.BlipType == SpoonerBlip::Type::Coord)
        {
            bHasCoord = true;
            break;
        }
    if (!bHasCoord)
    {
        bool bAddCoord = false;
        AddOption(Language::Translate("menu.spooner.blip.option.add_coord.label"), bAddCoord);
        if (bAddCoord)
        {
            auto& spoocam = SpoonerCamera::camera;
            if (!spoocam.IsActive())
            {
                GTAentity myPed = PLAYER_PED_ID();
                sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(myPed.GetPosition(), Vector3(0, 0, myPed.GetHeading()));
            }
            else
            {
                Vector3 spawnPos = spoocam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 120.0f, 30.0f);
                sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(spawnPos, Vector3(0, 0, spoocam.GetRotation().z));
            }
            sub::Spooner::SelectedBlip->BlipType = SpoonerBlip::Type::Coord;
            sub::Spooner::SelectedBlip->Alpha = 255;
            sub::Spooner::SelectedBlip->Scale = 0.80f;
            BlipCustoms::RefreshBlip(*sub::Spooner::SelectedBlip);
            Menu::pendingSubmenu = SUB::SPOONER_BLIPS_COORDINBLIP;
        }
    }
    for (UINT i = 0; i < Databases::BlipDb.size(); i++)
    {
        auto& m = Databases::BlipDb[i];
        if (m.BlipType != SpoonerBlip::Type::Coord)
            continue;

        bool bBlipPressed = false;
        std::string displayName = m.label.empty() ? BlipIcon::vNames.at(m.Icon) : m.label;
        AddOption(displayName, bBlipPressed);
        if (bBlipPressed)
        {
            sub::Spooner::SelectedBlip = &m;
            Menu::pendingSubmenu = SUB::SPOONER_BLIPS_COORDINBLIP;
        }

        if (*Menu::activeOptionIndex == Menu::currentOptionCount)
        {
            m.m_selectedInSub = true;

            bool bShortcutDeletePressed;
            Keybinds::AddBindIB("menu_action", Language::Translate("menu.spooner.blip.option.delete_coord.label"));
            bShortcutDeletePressed = Keybinds::WasPressedThisFrame("menu_action");

            if (bShortcutDeletePressed)
                blipIndexInDbToDelete = i;
        }
    }

    AddBreak(Language::Translate("menu.spooner.blip.section.radial_section.label"));
    bool bHasRadial = false;
    for (auto& b : Databases::BlipDb)
        if (b.BlipType == SpoonerBlip::Type::Radial)
        {
            bHasRadial = true;
            break;
        }
    if (!bHasRadial)
    {
        bool bAddRadial = false;
        AddOption(Language::Translate("menu.spooner.blip.option.add_radial.label"), bAddRadial);
        if (bAddRadial)
        {
            auto& spoocam = SpoonerCamera::camera;
            if (!spoocam.IsActive())
            {
                GTAentity myPed = PLAYER_PED_ID();
                sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(myPed.GetPosition(), Vector3(0, 0, myPed.GetHeading()));
            }
            else
            {
                Vector3 spawnPos = spoocam.RaycastForCoord(Vector2(0.0f, 0.0f), 0, 120.0f, 30.0f);
                sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(spawnPos, Vector3(0, 0, spoocam.GetRotation().z));
            }
            sub::Spooner::SelectedBlip->BlipType = SpoonerBlip::Type::Radial;
            sub::Spooner::SelectedBlip->Alpha = 160;
            sub::Spooner::SelectedBlip->Scale = 0.80f;
            BlipCustoms::RefreshBlip(*sub::Spooner::SelectedBlip);
            Menu::pendingSubmenu = SUB::SPOONER_BLIPS_RADIALINBLIP;
        }
    }
    for (UINT i = 0; i < Databases::BlipDb.size(); i++)
    {
        auto& m = Databases::BlipDb[i];
        if (m.BlipType != SpoonerBlip::Type::Radial)
            continue;

        bool bBlipPressed = false;
        std::string displayName = m.label.empty() ? Language::Translate("menu.spooner.blip.option.radial.label") : m.label;
        AddOption(displayName, bBlipPressed);
        if (bBlipPressed)
        {
            sub::Spooner::SelectedBlip = &m;
            Menu::pendingSubmenu = SUB::SPOONER_BLIPS_RADIALINBLIP;
        }

        if (*Menu::activeOptionIndex == Menu::currentOptionCount)
        {
            m.m_selectedInSub = true;

            bool bShortcutDeletePressed;
            Keybinds::AddBindIB("menu_action", Language::Translate("menu.spooner.blip.option.delete_radial.label"));
            bShortcutDeletePressed = Keybinds::WasPressedThisFrame("menu_action");

            if (bShortcutDeletePressed)
                blipIndexInDbToDelete = i;
        }
    }

    if (blipIndexInDbToDelete != -1)
        BlipCustoms::RemoveBlip(blipIndexInDbToDelete);

    if (*Menu::activeOptionIndex > Menu::currentOptionCount)
        Menu::Up();
}

void Sub_Blip_Radial()
{
    AddTitle(Language::Translate("menu.spooner.blip.radial.title"));

    bool bDeletePressed = false;
    AddOption(Language::Translate("menu.spooner.blip.option.delete.label"), bDeletePressed);
    if (bDeletePressed)
    {
        sub::Spooner::BlipCustoms::RemoveBlip(*sub::Spooner::SelectedBlip);
        sub::Spooner::SelectedBlip = nullptr;
        Menu::SetPreviousMenu();
    }
}

void Sub_Blip_Entity()
{
    AddTitle(Language::Translate("menu.spooner.blip.entity.title"));
}

void Sub_Blip_Entity_Select()
{
    AddTitle(Language::Translate("menu.spooner.blip.select_entity.title"));

    if (Databases::EntityDb.empty())
    {
        AddOption(Language::Translate("menu.spooner.blip.option.no_entities.label"), null);
        return;
    }

    for (auto& e : Databases::EntityDb)
    {
        if (e.handle.Exists())
        {
            bool bEntityPressed = false;
            AddOption(e.hashName, bEntityPressed);
            if (bEntityPressed)
            {
                sub::Spooner::SelectedBlip = sub::Spooner::BlipCustoms::AddBlip(SpoonerBlip::Type::Entity, e.hashName);

                sub::Spooner::SelectedBlip->EntityHandle = e.handle.GetHandle();
                sub::Spooner::SelectedBlip->bAttached = true;
                sub::Spooner::SelectedBlip->Alpha = 255;
                sub::Spooner::SelectedBlip->Scale = 0.80f;
                sub::Spooner::SelectedBlip->Icon = BlipIcon::Standard;
                BlipCustoms::RefreshBlip(*sub::Spooner::SelectedBlip);
                Menu::pendingSubmenu = SUB::SPOONER_BLIPS_ENTITYINBLIP;
                return;
            }

            if (*Menu::activeOptionIndex == Menu::currentOptionCount)
                EntityManagement::ShowArrowAboveEntity(e.handle, RGBA(0, 255, 0, 200));
        }
        else
        {
            AddOption(Language::Format("menu.spooner.multi_select.option.invalid_entity.label", {{"name", e.hashName}}), null);
        }
    }
}

void Sub_Blip_EntityInBlip()
{
    if (sub::Spooner::SelectedBlip == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    auto blip = sub::Spooner::SelectedBlip;

    AddTitle(Language::Translate("menu.spooner.blip.entity_options.title"));

    bool bEditLabelPressed = false;
    AddTexter(Language::Translate("menu.spooner.option.blip.label"), 0, std::vector<std::string>{blip->label.empty() ? "" : blip->label}, bEditLabelPressed);
    if (bEditLabelPressed)
    {
        blip->label = Game::InputBox(blip->label, 26U, Language::Translate("menu.spooner.blip.option.label_prompt.prompt").c_str(), blip->label);
        BlipCustoms::RefreshBlip(*blip);
    }

    bool colour_plus = false;
    bool colour_minus = false;

    AddTexter(Language::Translate("menu.spooner.blip.option.colour.label"), 0, std::vector<std::string>{BlipColour::vNames.at(blip->Colour)}, null, colour_plus, colour_minus);

    if (colour_plus)
    {
        auto it = BlipColour::vNames.find(blip->Colour);
        if (std::next(it) != BlipColour::vNames.end())
        {
            ++it;
            blip->Colour = it->first;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    if (colour_minus)
    {
        auto it = BlipColour::vNames.find(blip->Colour);
        if (it != BlipColour::vNames.begin())
        {
            --it;
            blip->Colour = it->first;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    AddOption(Language::Format("menu.spooner.blip.option.icon.label", {{"icon", BlipIcon::vNames.at(blip->Icon)}}), null, nullFunc, SUB::SPOONER_BLIPS_ICONS);

    bool scale_plus = false;
    bool scale_minus = false;

    AddNumber(Language::Translate("menu.spooner.blip.option.scale.label"), blip->Scale, 2, null, scale_plus, scale_minus);

    if (scale_plus)
    {
        if (blip->Scale < 10.0f)
        {
            blip->Scale += 0.1f;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    if (scale_minus)
    {
        if (blip->Scale > 0.1f)
        {
            blip->Scale -= 0.1f;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    bool bToggleRotation = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.sync_rotation.label"), blip->bSyncRotation, bToggleRotation, bToggleRotation, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.sync_rotation.description"));
    if (bToggleRotation)
    {
        blip->bSyncRotation = !blip->bSyncRotation;
        if (!blip->bSyncRotation)
            BlipCustoms::RefreshBlip(*blip);
    }

    bool bToggleRoute = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.show_route.label"), blip->bShowRoute, bToggleRoute, bToggleRoute, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    if (bToggleRoute)
    {
        blip->bShowRoute = !blip->bShowRoute;
        HUD::SET_BLIP_ROUTE(blip->BlipHandle, blip->bShowRoute);
    }

    if (blip->bShowRoute)
    {
        bool routeColour_plus = false;
        bool routeColour_minus = false;

        AddTexter(Language::Translate("menu.spooner.blip.option.route_colour.label"), 0, std::vector<std::string>{BlipColour::vNames.at(blip->RouteColour)}, null, routeColour_plus, routeColour_minus);

        if (routeColour_plus)
        {
            auto it = BlipColour::vNames.find(blip->RouteColour);
            if (std::next(it) != BlipColour::vNames.end())
                ++it;
            else
                it = BlipColour::vNames.begin();
            blip->RouteColour = it->first;
            HUD::SET_BLIP_ROUTE_COLOUR(blip->BlipHandle, blip->RouteColour);
        }

        if (routeColour_minus)
        {
            auto it = BlipColour::vNames.find(blip->RouteColour);
            if (it != BlipColour::vNames.begin())
                --it;
            else
                it = std::prev(BlipColour::vNames.end());
            blip->RouteColour = it->first;
            HUD::SET_BLIP_ROUTE_COLOUR(blip->BlipHandle, blip->RouteColour);
        }
    }

    bool bToggleCone = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.show_cone.label"), blip->bShowCone, bToggleCone, bToggleCone, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.show_cone.description"));
    if (bToggleCone)
    {
        blip->bShowCone = !blip->bShowCone;
        HUD::SET_BLIP_SHOW_CONE(blip->BlipHandle, blip->bShowCone, 9);
    }

    bool bToggleShortRange = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.short_range.label"), blip->bShortRange, bToggleShortRange, bToggleShortRange, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.short_range.description"));
    if (bToggleShortRange)
    {
        blip->bShortRange = !blip->bShortRange;
        HUD::SET_BLIP_AS_SHORT_RANGE(blip->BlipHandle, blip->bShortRange);
    }

    bool bToggleSelectable = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.selectable.label"), blip->bSelectableOnMap, bToggleSelectable, bToggleSelectable, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.selectable.description"));
    if (bToggleSelectable)
    {
        blip->bSelectableOnMap = !blip->bSelectableOnMap;
        HUD::SET_BLIP_DISPLAY(blip->BlipHandle, blip->bSelectableOnMap ? 2 : 8);
    }

    bool priority_plus = false;
    bool priority_minus = false;

    static const std::vector<int> priorityValues = {2, 3, 5, 6, 7, 9};
    auto priorityIt = std::find(priorityValues.begin(), priorityValues.end(), blip->Priority);
    if (priorityIt == priorityValues.end())
        priorityIt = priorityValues.begin();

    AddTexter(Language::Translate("menu.spooner.blip.option.priority.label"), 0, std::vector<std::string>{std::to_string(blip->Priority)}, null, priority_plus, priority_minus);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.priority.description"));

    if (priority_plus)
    {
        if (std::next(priorityIt) != priorityValues.end())
            ++priorityIt;
        else
            priorityIt = priorityValues.begin();
        blip->Priority = *priorityIt;
        HUD::SET_BLIP_PRIORITY(blip->BlipHandle, blip->Priority);
    }

    if (priority_minus)
    {
        if (priorityIt != priorityValues.begin())
            --priorityIt;
        else
            priorityIt = std::prev(priorityValues.end());
        blip->Priority = *priorityIt;
        HUD::SET_BLIP_PRIORITY(blip->BlipHandle, blip->Priority);
    }

    bool deletePressed = false;

    AddOption(Language::Translate("menu.spooner.blip.option.delete.label"), deletePressed);

    if (deletePressed)
    {
        BlipCustoms::RemoveBlip(*blip);
        sub::Spooner::SelectedBlip = nullptr;
        Menu::SetPreviousMenu();
        return;
    }
}

void Sub_Blip_Coord()
{
    AddTitle(Language::Translate("menu.spooner.blip.coord.title"));
}

void Sub_Blip_CoordInBlip()
{
    if (sub::Spooner::SelectedBlip == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    auto blip = sub::Spooner::SelectedBlip;

    AddTitle(Language::Translate("menu.spooner.blip.coord_options.title"));

    bool bEditLabelPressed = false;
    AddTexter(Language::Translate("menu.spooner.option.blip.label"), 0, std::vector<std::string>{blip->label.empty() ? "" : blip->label}, bEditLabelPressed);
    if (bEditLabelPressed)
    {
        blip->label = Game::InputBox(blip->label, 26U, Language::Translate("menu.spooner.blip.option.name_prompt.prompt").c_str(), blip->label);
        BlipCustoms::RefreshBlip(*blip);
    }

    bool colour_plus = false;
    bool colour_minus = false;

    AddTexter(Language::Translate("menu.spooner.blip.option.colour.label"), 0, std::vector<std::string>{BlipColour::vNames.at(blip->Colour)}, null, colour_plus, colour_minus);

    if (colour_plus)
    {
        auto it = BlipColour::vNames.find(blip->Colour);
        if (std::next(it) != BlipColour::vNames.end())
        {
            ++it;
            blip->Colour = it->first;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    if (colour_minus)
    {
        auto it = BlipColour::vNames.find(blip->Colour);
        if (it != BlipColour::vNames.begin())
        {
            --it;
            blip->Colour = it->first;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    AddOption(Language::Format("menu.spooner.blip.option.icon.label", {{"icon", BlipIcon::vNames.at(blip->Icon)}}), null, nullFunc, SUB::SPOONER_BLIPS_ICONS);

    bool scale_plus = false;
    bool scale_minus = false;

    AddNumber(Language::Translate("menu.spooner.blip.option.scale.label"), blip->Scale, 2, null, scale_plus, scale_minus);

    if (scale_plus)
    {
        if (blip->Scale < 10.0f)
        {
            blip->Scale += 0.1f;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    if (scale_minus)
    {
        if (blip->Scale > 0.1f)
        {
            blip->Scale -= 0.1f;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    bool alpha_plus = false;
    bool alpha_minus = false;

    AddNumber(Language::Translate("menu.spooner.blip.option.alpha.label"), blip->Alpha, 2, null, alpha_plus, alpha_minus);

    if (alpha_plus)
    {
        if (blip->Alpha < 255)
        {
            blip->Alpha++;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    if (alpha_minus)
    {
        if (blip->Alpha > 0)
        {
            blip->Alpha--;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    bool bToggleRoute = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.show_route.label"), blip->bShowRoute, bToggleRoute, bToggleRoute, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    if (bToggleRoute)
    {
        blip->bShowRoute = !blip->bShowRoute;
        HUD::SET_BLIP_ROUTE(blip->BlipHandle, blip->bShowRoute);
    }

    bool bToggleShortRange = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.short_range.label"), blip->bShortRange, bToggleShortRange, bToggleShortRange, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.short_range.description"));
    if (bToggleShortRange)
    {
        blip->bShortRange = !blip->bShortRange;
        HUD::SET_BLIP_AS_SHORT_RANGE(blip->BlipHandle, blip->bShortRange);
    }

    bool bToggleSelectable = false;
    AddTickol(Language::Translate("menu.spooner.blip.option.selectable.label"), blip->bSelectableOnMap, bToggleSelectable, bToggleSelectable, TICKOL::BOXTICK, TICKOL::BOXBLANK);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.selectable.description"));
    if (bToggleSelectable)
    {
        blip->bSelectableOnMap = !blip->bSelectableOnMap;
        HUD::SET_BLIP_DISPLAY(blip->BlipHandle, blip->bSelectableOnMap ? 2 : 8);
    }

    bool priority_plus = false;
    bool priority_minus = false;

    static const std::vector<int> priorityValues = {2, 3, 5, 6, 7, 9};
    auto priorityIt = std::find(priorityValues.begin(), priorityValues.end(), blip->Priority);
    if (priorityIt == priorityValues.end())
        priorityIt = priorityValues.begin();

    AddTexter(Language::Translate("menu.spooner.blip.option.priority.label"), 0, std::vector<std::string>{std::to_string(blip->Priority)}, null, priority_plus, priority_minus);
    AddOptionDescription(Language::Translate("menu.spooner.blip.option.priority.description"));

    if (priority_plus)
    {
        if (std::next(priorityIt) != priorityValues.end())
            ++priorityIt;
        else
            priorityIt = priorityValues.begin();
        blip->Priority = *priorityIt;
        HUD::SET_BLIP_PRIORITY(blip->BlipHandle, blip->Priority);
    }

    if (priority_minus)
    {
        if (priorityIt != priorityValues.begin())
            --priorityIt;
        else
            priorityIt = std::prev(priorityValues.end());
        blip->Priority = *priorityIt;
        HUD::SET_BLIP_PRIORITY(blip->BlipHandle, blip->Priority);
    }

    bool deletePressed = false;

    AddOption(Language::Translate("menu.spooner.blip.option.delete.label"), deletePressed);

    if (deletePressed)
    {
        BlipCustoms::RemoveBlip(*blip);
        sub::Spooner::SelectedBlip = nullptr;
        Menu::SetPreviousMenu();
        return;
    }

    AddBreak(Language::Translate("menu.spooner.blip.section.position.label"));
    {
        AddOption("~italic~" + Vector3(blip->X, blip->Y, blip->Z).ToString(), null);

        bool bSetToPlayer = false;
        AddOption(Language::Translate("menu.spooner.marker.option.set_to_player_position.label"), bSetToPlayer);
        if (bSetToPlayer)
        {
            Vector3 pos = ENTITY::GET_ENTITY_COORDS(PLAYER_PED_ID(), true);
            blip->X = pos.x;
            blip->Y = pos.y;
            blip->Z = pos.z;
            BlipCustoms::RefreshBlip(*blip);
        }

        if (IS_WAYPOINT_ACTIVE())
        {
            bool bSetToWp = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_waypoint.label"), bSetToWp);
            if (bSetToWp)
            {
                Blip wp = GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint);
                Vector3 wpCoords = GET_BLIP_COORDS(wp);
                wpCoords.z = World::GetGroundHeight(wpCoords);

                blip->X = wpCoords.x;
                blip->Y = wpCoords.y;
                blip->Z = wpCoords.z;
                BlipCustoms::RefreshBlip(*blip);
            }
        }

        bool bManual = false;
        AddOption(Language::Translate("menu.spooner.blip.option.manual_placement.label"), bManual, nullFunc, SUB::SPOONER_VECTOR3_MANUALPLACEMENT);
        if (bManual)
        {
            SpoonerVector3ManualEditingPtrs = std::make_tuple((GTAentity)0, &blip->Offset, (Vector3*)nullptr);
        }
    }
}

void Sub_Blip_RadialInBlip()
{
    if (sub::Spooner::SelectedBlip == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    auto blip = sub::Spooner::SelectedBlip;

    AddTitle(Language::Translate("menu.spooner.blip.radial_options.title"));

    bool shape_plus = false;
    bool shape_minus = false;

    std::string shapeName =
        (blip->Shape == SpoonerBlip::RadialShape::Circle) ? Language::Translate("menu.spooner.blip.option.circle.label") : Language::Translate("menu.spooner.blip.option.square.label");

    AddTexter(Language::Translate("menu.spooner.blip.option.shape.label"), 0, std::vector<std::string>{shapeName}, null, shape_plus, shape_minus);

    if (shape_plus || shape_minus)
    {
        blip->Shape = (blip->Shape == SpoonerBlip::RadialShape::Circle) ? SpoonerBlip::RadialShape::Square : SpoonerBlip::RadialShape::Circle;
        sub::Spooner::BlipCustoms::RefreshBlip(*blip);
    }

    //Circle options
    if (blip->Shape == SpoonerBlip::RadialShape::Circle)
    {
        bool radius_plus = false;
        bool radius_minus = false;

        AddNumber(Language::Translate("menu.spooner.blip.option.radius.label"), blip->RadialSize, 2, null, radius_plus, radius_minus);

        if (radius_plus)
        {
            if (blip->RadialSize < 5000.0f)
            {
                blip->RadialSize += 1.0f;
                sub::Spooner::BlipCustoms::RefreshBlip(*blip);
            }
        }

        if (radius_minus)
        {
            if (blip->RadialSize > 1.0f)
            {
                blip->RadialSize -= 1.0f;
                sub::Spooner::BlipCustoms::RefreshBlip(*blip);
            }
        }
    }

    //Square options
    if (blip->Shape == SpoonerBlip::RadialShape::Square)
    {
        bool width_plus = false;
        bool width_minus = false;

        AddNumber(Language::Translate("menu.spooner.blip.option.width.label"), blip->AreaWidth, 2, null, width_plus, width_minus);

        if (width_plus)
        {
            if (blip->AreaWidth < 5000.0f)
            {
                blip->AreaWidth += 1.0f;
                sub::Spooner::BlipCustoms::RefreshBlip(*blip);
            }
        }

        if (width_minus)
        {
            if (blip->AreaWidth > 1.0f)
            {
                blip->AreaWidth -= 1.0f;
                sub::Spooner::BlipCustoms::RefreshBlip(*blip);
            }
        }

        bool height_plus = false;
        bool height_minus = false;

        AddNumber(Language::Translate("menu.spooner.blip.option.height.label"), blip->AreaHeight, 2, null, height_plus, height_minus);

        if (height_plus)
        {
            if (blip->AreaHeight < 5000.0f)
            {
                blip->AreaHeight += 1.0f;
                sub::Spooner::BlipCustoms::RefreshBlip(*blip);
            }
        }

        if (height_minus)
        {
            if (blip->AreaHeight > 1.0f)
            {
                blip->AreaHeight -= 1.0f;
                sub::Spooner::BlipCustoms::RefreshBlip(*blip);
            }
        }
        bool heading_plus = false;
        bool heading_minus = false;

        AddNumber(Language::Translate("menu.spooner.blip.option.rotation.label"), blip->Heading, 2, null, heading_plus, heading_minus);

        if (heading_plus)
        {
            if (blip->Heading < 90.0f)
            {
                blip->Heading += 1.0f;
                sub::Spooner::BlipCustoms::RefreshBlip(*blip);
            }
        }

        if (heading_minus)
        {
            if (blip->Heading > 0.0f)
            {
                blip->Heading -= 1.0f;
                sub::Spooner::BlipCustoms::RefreshBlip(*blip);
            }
        }
    }

    bool colour_plus = false;
    bool colour_minus = false;

    AddTexter(Language::Translate("menu.spooner.blip.option.colour.label"), 0, std::vector<std::string>{BlipColour::vNames.at(blip->Colour)}, null, colour_plus, colour_minus);

    if (colour_plus)
    {
        auto it = BlipColour::vNames.find(blip->Colour);
        if (std::next(it) != BlipColour::vNames.end())
        {
            ++it;
            blip->Colour = it->first;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    if (colour_minus)
    {
        auto it = BlipColour::vNames.find(blip->Colour);
        if (it != BlipColour::vNames.begin())
        {
            --it;
            blip->Colour = it->first;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    bool alpha_plus = false;
    bool alpha_minus = false;

    AddNumber(Language::Translate("menu.spooner.blip.option.alpha.label"), blip->Alpha, 2, null, alpha_plus, alpha_minus);

    if (alpha_plus)
    {
        if (blip->Alpha < 255)
        {
            blip->Alpha++;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    if (alpha_minus)
    {
        if (blip->Alpha > 0)
        {
            blip->Alpha--;
            sub::Spooner::BlipCustoms::RefreshBlip(*blip);
        }
    }

    bool deletePressed = false;

    AddOption(Language::Translate("menu.spooner.blip.option.delete.label"), deletePressed);

    if (deletePressed)
    {
        BlipCustoms::RemoveBlip(*blip);
        sub::Spooner::SelectedBlip = nullptr;
        Menu::SetPreviousMenu();
        return;
    }

    AddBreak(Language::Translate("menu.spooner.blip.section.position.label"));
    {
        AddOption("~italic~" + Vector3(blip->X, blip->Y, blip->Z).ToString(), null);

        bool bSetToPlayer = false;
        AddOption(Language::Translate("menu.spooner.marker.option.set_to_player_position.label"), bSetToPlayer);
        if (bSetToPlayer)
        {
            Vector3 pos = ENTITY::GET_ENTITY_COORDS(PLAYER_PED_ID(), true);
            blip->X = pos.x;
            blip->Y = pos.y;
            blip->Z = pos.z;
            blip->EntityHandle = 0;
            blip->bAttached = false;
            BlipCustoms::RefreshBlip(*blip);
        }

        if (IS_WAYPOINT_ACTIVE())
        {
            bool bSetToWp = false;
            AddOption(Language::Translate("menu.spooner.marker.option.set_to_waypoint.label"), bSetToWp);
            if (bSetToWp)
            {
                Blip wp = GET_FIRST_BLIP_INFO_ID(BlipIcon::Waypoint);
                Vector3 wpCoords = GET_BLIP_COORDS(wp);
                wpCoords.z = World::GetGroundHeight(wpCoords);

                blip->X = wpCoords.x;
                blip->Y = wpCoords.y;
                blip->Z = wpCoords.z;
                blip->EntityHandle = 0;
                blip->bAttached = false;
                BlipCustoms::RefreshBlip(*blip);
            }
        }

        AddOption(Language::Translate("menu.spooner.marker.option.attach_to_entity.label"), null, nullFunc, SUB::SPOONER_BLIPS_ATTACH);

        if (blip->bAttached)
        {
            bool bAdjust = false;
            AddOption(Language::Translate("menu.spooner.marker.option.adjust_attachment.label"), bAdjust, nullFunc, SUB::SPOONER_VECTOR3_MANUALPLACEMENT);
            if (bAdjust)
            {
                SpoonerVector3ManualEditingPtrs = std::make_tuple((GTAentity)blip->EntityHandle, &blip->Offset, (Vector3*)nullptr);
            }
        }
        else
        {
            bool bManual = false;
            AddOption(Language::Translate("menu.spooner.blip.option.manual_placement.label"), bManual, nullFunc, SUB::SPOONER_VECTOR3_MANUALPLACEMENT);
            if (bManual)
            {
                SpoonerVector3ManualEditingPtrs = std::make_tuple((GTAentity)0, &blip->Offset, (Vector3*)nullptr);
            }
        }
    }
}

void Sub_Blip_Attach()
{
    if (sub::Spooner::SelectedBlip == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    auto blip = sub::Spooner::SelectedBlip;

    AddTitle(Language::Translate("menu.spooner.attachment.title"));

    bool bDetachPressed = false;
    AddTickol(Language::Translate("menu.spooner.attachment.option.detach.label"), !blip->bAttached, bDetachPressed, bDetachPressed, TICKOL::TICK2);
    if (bDetachPressed)
    {
        if (blip->bAttached)
        {
            Vector3 worldPos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(blip->EntityHandle, blip->Offset.x, blip->Offset.y, blip->Offset.z);
            blip->X = worldPos.x;
            blip->Y = worldPos.y;
            blip->Z = worldPos.z;
        }
        blip->EntityHandle = 0;
        blip->bAttached = false;
        blip->Offset.clear();
        BlipCustoms::RefreshBlip(*blip);
    }

    if (!Databases::EntityDb.empty())
    {
        AddBreak(Language::Translate("menu.spooner.attachment.section.database.label"));
        for (auto& e : Databases::EntityDb)
        {
            if (e.handle.Exists())
            {
                bool bEntityPressed = false;
                AddTickol(e.hashName, blip->EntityHandle == e.handle.GetHandle(), bEntityPressed, bEntityPressed, TICKOL::TICK2);
                if (bEntityPressed)
                {
                    blip->EntityHandle = e.handle.GetHandle();
                    blip->Offset.clear();
                    blip->bAttached = true;
                    BlipCustoms::RefreshBlip(*blip);
                    Menu::SetPreviousMenu();
                    return;
                }

                if (*Menu::activeOptionIndex == Menu::currentOptionCount)
                    EntityManagement::ShowArrowAboveEntity(e.handle, RGBA(0, 255, 0, 200));
            }
            else
            {
                AddOption(Language::Format("menu.spooner.multi_select.option.invalid_entity.label", {{"name", e.hashName}}), null);
            }
        }
    }
}

void Sub_Blip_Icons()
{
    if (sub::Spooner::SelectedBlip == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    AddTitle(Language::Translate("menu.spooner.blip.icons.title"));

    bool bSearchPressed = false;
    AddOption(_searchStr.empty() ? Language::Translate("component.search.action.label") : boost::to_upper_copy(_searchStr), bSearchPressed, nullFunc, -1, true);
    if (bSearchPressed)
    {
        _searchStr = Game::InputBox(_searchStr, 126U, Language::Translate("component.search.prompt").c_str(), _searchStr);
        boost::to_lower(_searchStr);
    }

    static int sortMode = 0;
    bool sort_plus = false;
    bool sort_minus = false;
    AddTexter(
        Language::Translate("menu.spooner.blip.option.sort.label"),
        0,
        std::vector<std::string>{sortMode == 0 ? Language::Translate("menu.spooner.option.blip.value.sort_regular") : Language::Translate("menu.spooner.option.blip.value.sort_alphabetic")},
        null,
        sort_plus,
        sort_minus
    );
    if (sort_plus || sort_minus)
        sortMode = sortMode == 0 ? 1 : 0;

    std::vector<std::pair<int, std::string>> entries(BlipIcon::vNames.begin(), BlipIcon::vNames.end());

    if (!_searchStr.empty())
    {
        entries.erase(
            std::remove_if(
                entries.begin(),
                entries.end(),
                [](const std::pair<int, std::string>& e)
                {
                    std::string nameLower = boost::to_lower_copy(e.second);
                    return nameLower.find(_searchStr) == std::string::npos;
                }
            ),
            entries.end()
        );
    }

    if (sortMode == 1)
    {
        std::sort(entries.begin(), entries.end(), [](const std::pair<int, std::string>& a, const std::pair<int, std::string>& b) { return a.second < b.second; });
    }

    std::vector<int> favourites;
    FavouritesManagement::GetFavouriteBlipIcons(favourites);

    std::vector<std::pair<int, std::string>> favEntries, otherEntries;
    for (auto& e : entries)
    {
        if (std::find(favourites.begin(), favourites.end(), e.first) != favourites.end())
            favEntries.push_back(e);
        else
            otherEntries.push_back(e);
    }

    if (!favEntries.empty())
        AddBreak(Language::Translate("menu.spooner.blip.section.favourites.label"));

    for (const auto& [icon, name] : favEntries)
    {
        bool bIconPressed = false;
        AddTickol(name, sub::Spooner::SelectedBlip->Icon == icon, bIconPressed, bIconPressed, TICKOL::TICK2);

        if (Menu::currentOptionCount == *Menu::activeOptionIndex)
        {
            sub::Spooner::SelectedBlip->Icon = icon;
            sub::Spooner::BlipCustoms::RefreshBlip(*sub::Spooner::SelectedBlip);
            Keybinds::AddBindIB("menu_action", Language::Translate("common.remove_from_favourites"));
            if (Keybinds::WasPressedThisFrame("menu_action"))
                FavouritesManagement::RemoveBlipIconFromFavourites(icon);
        }
    }

    if (!otherEntries.empty())
        AddBreak(Language::Translate("menu.spooner.blip.section.all_icons.label"));

    for (const auto& [icon, name] : otherEntries)
    {
        bool bIconPressed = false;
        AddTickol(name, sub::Spooner::SelectedBlip->Icon == icon, bIconPressed, bIconPressed, TICKOL::TICK2);

        if (Menu::currentOptionCount == *Menu::activeOptionIndex)
        {
            sub::Spooner::SelectedBlip->Icon = icon;
            sub::Spooner::BlipCustoms::RefreshBlip(*sub::Spooner::SelectedBlip);
            Keybinds::AddBindIB("menu_action", Language::Translate("common.add_to_favourites"));
            if (Keybinds::WasPressedThisFrame("menu_action"))
                FavouritesManagement::AddBlipIconToFavourites(icon);
        }
    }
}

void Sub_SpawnCategories()
{
    _searchStr.clear();

    AddTitle(Language::Translate("menu.spooner.spawn.title"));
    AddOption(Language::Translate("menu.spooner.spawn.option.object.label"), null, nullFunc, SUB::SPOONER_SPAWN_PROP);
    AddOption(Language::Translate("menu.spooner.spawn.option.ped.label"), null, nullFunc, SUB::SPOONER_SPAWN_PED);
    AddOption(Language::Translate("menu.spooner.spawn.option.vehicle.label"), null, nullFunc, SUB::SPOONER_SPAWN_VEHICLE);
}

namespace ObjectSearch
{
static constexpr size_t MaxVisibleResults = 500;
static int sortIndex = 0;
static int categoryFilter = 0;
static int dlcFilter = 0;
static int stuntFilter = 0;
static int lodFilter = 0;
static int sizeFilter = 0;
static std::vector<std::string> results;
static std::vector<std::string> categoryOptions;
static std::vector<std::string> dlcOptions;
static std::vector<std::string> stuntOptions;
static std::vector<std::string> sizeOptions;
static std::string lastSearch = "\x01";
static int lastSort = -1, lastCategory = -1, lastDlc = -1, lastStunt = -1, lastLod = -1, lastSize = -1;
static size_t lastObjectCount = 0;
static bool dirty = true;

static const std::vector<std::string> sortOptionsList = {"Name (A-Z)", "Name (Z-A)"};
static const std::vector<std::string> lodOptionsList = {"Hide LOD", "Show All"};

void BuildCategoryOptions()
{
    std::array<int, ObjectCategories::Category_Count> counts = {};
    for (const auto& modelName : objectModels)
    {
        counts[ObjectCategories::GetObjectCategory(modelName)]++;
    }

    categoryOptions.clear();
    categoryOptions.push_back("All (" + std::to_string(objectModels.size()) + ")");
    for (int i = 0; i < ObjectCategories::Category_Count; i++)
    {
        const auto category = static_cast<ObjectCategories::Category>(i);
        categoryOptions.push_back(std::string(ObjectCategories::GetObjectCategoryName(category)) + " (" + std::to_string(counts[i]) + ")");
    }
}

void BuildDlcOptions()
{
    std::array<int, ObjectCategories::DLC_Count> counts = {};
    for (const auto& modelName : objectModels)
    {
        counts[ObjectCategories::GetObjectDlcGroup(modelName)]++;
    }

    dlcOptions.clear();
    dlcOptions.push_back("All (" + std::to_string(objectModels.size()) + ")");
    for (int i = 0; i < ObjectCategories::DLC_Count; i++)
    {
        const auto group = static_cast<ObjectCategories::DlcGroup>(i);
        dlcOptions.push_back(std::string(ObjectCategories::GetObjectDlcGroupName(group)) + " (" + std::to_string(counts[i]) + ")");
    }
}

void BuildStuntOptions()
{
    std::array<int, ObjectCategories::Stunt_Count> counts = {};
    for (const auto& modelName : objectModels)
    {
        auto st = ObjectCategories::GetStuntType(modelName);
        if (st != ObjectCategories::Stunt_None)
            counts[st]++;
    }

    int totalStunt = 0;
    for (int i = 1; i < ObjectCategories::Stunt_Count; i++)
        totalStunt += counts[i];

    stuntOptions.clear();
    stuntOptions.push_back("All");
    stuntOptions.push_back("All Stunt (" + std::to_string(totalStunt) + ")");
    for (int i = 1; i < ObjectCategories::Stunt_Count; i++)
    {
        if (counts[i] > 0)
            stuntOptions.push_back(std::string(ObjectCategories::GetStuntTypeName(static_cast<ObjectCategories::StuntType>(i))) + " (" + std::to_string(counts[i]) + ")");
        else
            stuntOptions.push_back(ObjectCategories::GetStuntTypeName(static_cast<ObjectCategories::StuntType>(i)));
    }
}

void BuildSizeOptions()
{
    std::array<int, ObjectCategories::Size_Count> counts = {};
    for (const auto& modelName : objectModels)
    {
        auto sz = ObjectCategories::GetNamedSize(modelName);
        if (sz != ObjectCategories::Size_Unknown)
            counts[sz]++;
    }

    int totalSized = 0;
    for (int i = 1; i < ObjectCategories::Size_Count; i++)
        totalSized += counts[i];

    sizeOptions.clear();
    sizeOptions.push_back("All");
    sizeOptions.push_back("Has Size (" + std::to_string(totalSized) + ")");
    for (int i = 1; i < ObjectCategories::Size_Count; i++)
    {
        auto sz = static_cast<ObjectCategories::SizeClass>(i);
        sizeOptions.push_back(std::string(ObjectCategories::GetSizeClassName(sz)) + " (" + std::to_string(counts[i]) + ")");
    }
}

void EnsureOptions()
{
    if (categoryOptions.empty() || dlcOptions.empty() || stuntOptions.empty() || sizeOptions.empty() || lastObjectCount != objectModels.size())
    {
        BuildCategoryOptions();
        BuildDlcOptions();
        BuildStuntOptions();
        BuildSizeOptions();
        lastObjectCount = objectModels.size();
        dirty = true;
    }
}

void RebuildResults(const std::string& searchStr)
{
    results.clear();
    std::string searchUpper = boost::to_upper_copy(searchStr);

    for (const auto& modelName : objectModels)
    {
        if (lodFilter == 0 && ObjectCategories::IsLodProxy(modelName))
            continue;

        if (categoryFilter != 0)
        {
            const auto category = ObjectCategories::GetObjectCategory(modelName);
            if (static_cast<int>(category) != (categoryFilter - 1))
                continue;
        }

        if (dlcFilter != 0)
        {
            const auto dlcGroup = ObjectCategories::GetObjectDlcGroup(modelName);
            if (static_cast<int>(dlcGroup) != (dlcFilter - 1))
                continue;
        }

        if (stuntFilter != 0)
        {
            auto st = ObjectCategories::GetStuntType(modelName);
            if (stuntFilter == 1)
            {
                if (st == ObjectCategories::Stunt_None)
                    continue;
            }
            else
            {
                if (static_cast<int>(st) != (stuntFilter - 1))
                    continue;
            }
        }

        if (sizeFilter != 0)
        {
            auto sz = ObjectCategories::GetNamedSize(modelName);
            if (sizeFilter == 1)
            {
                if (sz == ObjectCategories::Size_Unknown)
                    continue;
            }
            else
            {
                if (static_cast<int>(sz) != (sizeFilter - 1))
                    continue;
            }
        }

        if (!searchUpper.empty())
        {
            std::string modelUpper = boost::to_upper_copy(modelName);
            if (modelUpper.find(searchUpper) == std::string::npos)
                continue;
        }

        results.push_back(modelName);
    }

    switch (sortIndex)
    {
    case 0:
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return boost::to_lower_copy(a) < boost::to_lower_copy(b); });
        break;
    case 1:
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return boost::to_lower_copy(a) > boost::to_lower_copy(b); });
        break;
    }
}
} // namespace ObjectSearch

void ObjectSpawnerSearchMenu()
{
    using namespace ObjectSearch;
    auto& searchStr = dict;

    EnsureOptions();

    AddTitle(Language::Translate("menu.spooner.object_search.title"));

    bool bSearchPressed = false;
    AddOption(searchStr.empty() ? "~b~" + Language::Translate("component.search.action.label") + "~s~" : ("~b~" + searchStr + "~s~"), bSearchPressed, nullFunc, -1, true);
    if (bSearchPressed)
    {
        searchStr = Game::InputBox(searchStr, 64U, Language::Translate("menu.spooner.option.object_search.prompt").c_str(), boost::to_lower_copy(searchStr));
        boost::to_upper(searchStr);
    }

    bool sortRight = false, sortLeft = false;
    AddTexter(
        Language::Translate("menu.spooner.object_search.option.sort.label"),
        sortIndex,
        {Language::Translate("menu.spooner.option.object_search.value.sort_name_asc"), Language::Translate("menu.spooner.option.object_search.value.sort_name_desc")},
        null,
        sortRight,
        sortLeft
    );
    if (sortRight && sortIndex < (int)(sortOptionsList.size() - 1))
        sortIndex++;
    if (sortLeft && sortIndex > 0)
        sortIndex--;

    bool catRight = false, catLeft = false;
    AddTexter(Language::Translate("menu.spooner.object_search.option.category.label"), categoryFilter, categoryOptions, null, catRight, catLeft);
    if (catRight && categoryFilter < (int)(categoryOptions.size() - 1))
        categoryFilter++;
    if (catLeft && categoryFilter > 0)
        categoryFilter--;

    bool dlcRight = false, dlcLeft = false;
    AddTexter(Language::Translate("menu.spooner.object_search.option.dlc.label"), dlcFilter, dlcOptions, null, dlcRight, dlcLeft);
    if (dlcRight && dlcFilter < (int)(dlcOptions.size() - 1))
        dlcFilter++;
    if (dlcLeft && dlcFilter > 0)
        dlcFilter--;

    bool stuntRight = false, stuntLeft = false;
    AddTexter(Language::Translate("menu.spooner.object_search.option.stunt.label"), stuntFilter, stuntOptions, null, stuntRight, stuntLeft);
    if (stuntRight && stuntFilter < (int)(stuntOptions.size() - 1))
        stuntFilter++;
    if (stuntLeft && stuntFilter > 0)
        stuntFilter--;

    bool sizeRight = false, sizeLeft = false;
    AddTexter(Language::Translate("menu.spooner.object_search.option.size.label"), sizeFilter, sizeOptions, null, sizeRight, sizeLeft);
    AddOptionDescription(Language::Translate("menu.spooner.object_search.option.size.description"));
    if (sizeRight && sizeFilter < (int)(sizeOptions.size() - 1))
        sizeFilter++;
    if (sizeLeft && sizeFilter > 0)
        sizeFilter--;

    bool lodRight = false, lodLeft = false;
    AddTexter(
        Language::Translate("menu.spooner.object_search.option.lod.label"),
        lodFilter,
        {Language::Translate("menu.spooner.option.object_search.value.hide_lod"), Language::Translate("menu.spooner.option.object_search.value.show_all")},
        null,
        lodRight,
        lodLeft
    );
    AddOptionDescription(Language::Translate("menu.spooner.object_search.option.lod.description"));
    if (lodRight && lodFilter < 1)
        lodFilter++;
    if (lodLeft && lodFilter > 0)
        lodFilter--;

    if (searchStr != lastSearch || sortIndex != lastSort || categoryFilter != lastCategory || dlcFilter != lastDlc || stuntFilter != lastStunt || lodFilter != lastLod || sizeFilter != lastSize)
    {
        dirty = true;
    }

    if (dirty)
    {
        RebuildResults(searchStr);
        lastSearch = searchStr;
        lastSort = sortIndex;
        lastCategory = categoryFilter;
        lastDlc = dlcFilter;
        lastStunt = stuntFilter;
        lastLod = lodFilter;
        lastSize = sizeFilter;
        dirty = false;
    }

    if (results.size() > MaxVisibleResults)
        AddBreak(Language::Format("menu.spooner.object_search.section.results_limited.label", {{"count", results.size()}, {"max", MaxVisibleResults}}));
    else
        AddBreak(Language::Format("menu.spooner.object_search.section.results.label", {{"count", results.size()}}));

    const size_t visibleCount = results.size() < MaxVisibleResults ? results.size() : MaxVisibleResults;
    for (size_t i = 0; i < visibleCount; i++)
    {
        const auto& modelName = results[i];
        Model currentModel = GET_HASH_KEY(modelName);

        MenuOptions::AddOption_AddProp(modelName, currentModel.hash);

        if (Menu::IsLastDrawnOptionSelected())
        {
            bool bIsAFav = FavouritesManagement::IsPropAFavourite(modelName, currentModel.hash);
            Keybinds::AddBindIB("menu_action", bIsAFav, Language::Translate("common.remove_from_favourites"), Language::Translate("common.add_to_favourites"));

            if (Keybinds::WasPressedThisFrame("menu_action"))
            {
                !bIsAFav ? FavouritesManagement::AddPropToFavourites(modelName, currentModel.hash) : FavouritesManagement::RemovePropFromFavourites(modelName, currentModel.hash);
            }
        }
    }
}

void Sub_SpawnProp()
{
    AddTitle(Language::Translate("menu.spooner.spawn.object_title.title"));

    AddOption(Language::Translate("menu.spooner.option.favourites.label"), null, nullFunc, SUB::SPOONER_SPAWN_PROP_FAVOURITES);
    bool bOpenObjectSearch = false;
    AddOption(Language::Translate("menu.spooner.object_search.option.open.label"), bOpenObjectSearch, nullFunc, SUB::OBJECTSPAWNER_SEARCH);
    if (bOpenObjectSearch)
        dict.clear();

    bool bSearchPressed = false;
    AddOption(_searchStr.empty() ? Language::Translate("component.search.action.label") : boost::to_upper_copy(_searchStr), bSearchPressed, nullFunc, -1, true);
    if (bSearchPressed)
    {
        _searchStr = Game::InputBox(_searchStr, 126U, Language::Translate("component.search.prompt").c_str(), _searchStr);
        boost::to_lower(_searchStr);
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SearchToLower, _searchStr, 126U, std::string(), _searchStr);
        //OnscreenKeyboard::State::arg1._ptr = reinterpret_cast<void*>(&_searchStr);
    }

    // Display props models loop
    for (const auto& current : objectModels)
    {
        if (!_searchStr.empty())
        {
            if (current.find(_searchStr) == std::string::npos)
                continue;
        }

        Model currentModel = GET_HASH_KEY(current);

        MenuOptions::AddOption_AddProp(current, currentModel.hash);

        if (Menu::IsLastDrawnOptionSelected())
        {
            bool bIsAFav = FavouritesManagement::IsPropAFavourite(current, currentModel.hash);
            Keybinds::AddBindIB("menu_action", bIsAFav, Language::Translate("common.remove_from_favourites"), Language::Translate("common.add_to_favourites"));

            if (Keybinds::WasPressedThisFrame("menu_action"))
            {
                !bIsAFav ? FavouritesManagement::AddPropToFavourites(current, currentModel.hash) : FavouritesManagement::RemovePropFromFavourites(current, currentModel.hash);
            }
        }
    }

    bool bInputPressed = false;
    AddOption(Language::Translate("menu.spooner.spawn.option.input_model.label"), bInputPressed);
    if (bInputPressed)
    {
        EntityManagement::InputEntityIntoDb(EntityType::PROP);
    }
}

void RebuildFavPropCache(const std::string& searchStr)
{
    s_favPropCache.byCategory.clear();
    s_favPropCache.sortedCategories.clear();

    using FavouritesManagement::xmlFavouriteProps;
    pugi::xml_document doc;
    if (doc.load_file((const char*)(GetPathffA(Pathff::Main, true) + xmlFavouriteProps).c_str()).status != pugi::status_ok)
        return;
    pugi::xml_node nodeRoot = doc.child("FavouriteProps");
    if (!nodeRoot)
        return;

    std::string searchUpper = boost::to_upper_copy(searchStr);

    for (auto node = nodeRoot.first_child(); node; node = node.next_sibling())
    {
        std::string modelName = node.attribute("modelName").as_string();
        if (!searchUpper.empty())
        {
            std::string nameUpper = boost::to_upper_copy(modelName);
            if (nameUpper.find(searchUpper) == std::string::npos)
                continue;
        }
        Model model = node.attribute("modelHash").as_uint(0);
        if (model.hash == 0)
            model = GET_HASH_KEY(modelName);
        std::string category = node.attribute("category").as_string("");
        s_favPropCache.byCategory[category].push_back({modelName, model, category});
    }

    for (auto& kv : s_favPropCache.byCategory)
        s_favPropCache.sortedCategories.push_back(kv.first);
    std::sort(
        s_favPropCache.sortedCategories.begin(),
        s_favPropCache.sortedCategories.end(),
        [](const std::string& a, const std::string& b)
        {
            if (a.empty())
                return false;
            if (b.empty())
                return true;
            return a < b;
        }
    );

    s_favPropCache.needsRebuild = false;
}

void Sub_SpawnProp_Favourites()
{
    AddTitle(Language::Translate("menu.spooner.favourites.title"));

    Menu::OnSubBack = []() { s_favPropCache.needsRebuild = true; };

    bool searchActive = !s_favPropSearchStr.empty();

    bool bSearchPressed = false;
    AddOption(s_favPropSearchStr.empty() ? "~b~" + Language::Translate("component.search.action.label") + "~s~" : ("~b~" + s_favPropSearchStr + "~s~"), bSearchPressed, nullFunc, -1, true);
    if (bSearchPressed)
    {
        s_favPropSearchStr = Game::InputBox(s_favPropSearchStr, 64U, Language::Translate("menu.spooner.favourites.option.search_prompt.prompt").c_str(), boost::to_lower_copy(s_favPropSearchStr));
        boost::to_upper(s_favPropSearchStr);
        s_favPropCache.needsRebuild = true;
    }

    if (s_favPropCache.needsRebuild)
        RebuildFavPropCache(s_favPropSearchStr);

    if (searchActive)
        MenuCategory::ExpandAll();
    else
        MenuCategory::RestoreExpandedState();

    bool bInputAdd = false;
    AddOption(Language::Translate("menu.spooner.favourites.option.add_object.label"), bInputAdd);
    if (bInputAdd)
    {
        std::string inputStr = Game::InputBox("", 40U, Language::Translate("menu.spooner.favourites.option.model_prompt.prompt").c_str());
        if (inputStr.length() > 0)
        {
            if (FavouritesManagement::AddPropToFavourites(inputStr, GET_HASH_KEY(inputStr)))
            {
                Game::Print::PrintBottomLeft(Language::Translate("message.spooner.favourites.model_added.message"));
                s_favPropCache.needsRebuild = true;
            }
            else
                Game::Print::PrintBottomLeft(Language::Translate("message.spooner.favourites.model_add_failed.message"));
        }
    }

    AddDivider();

    MenuCategory::ResetCategoryState();
    for (auto& cat : s_favPropCache.sortedCategories)
    {
        auto it = s_favPropCache.byCategory.find(cat);
        if (it == s_favPropCache.byCategory.end())
            continue;

        auto& props = it->second;
        std::string catName = cat.empty() ? "UNORDERED" : cat;
        std::string catLabel = "— ~b~" + catName + "~s~ ~c~(" + std::to_string(props.size()) + ")~s~";

        if (MenuCategory::AddCategory(catLabel))
        {
            for (auto& prop : props)
            {
                MenuOptions::AddOption_AddProp(prop.modelName, prop.model);

                if (Menu::IsLastDrawnOptionSelected())
                {
                    Keybinds::AddBindIB("menu_action", Language::Translate("common.remove"));
                    if (Keybinds::WasPressedThisFrame("menu_action"))
                    {
                        FavouritesManagement::RemovePropFromFavourites(prop.modelName, prop.model.hash);
                        s_favPropCache.needsRebuild = true;
                        if (Menu::IsSelectionAtBottom())
                            Menu::Up();
                        return;
                    }
                    Keybinds::AddBindIB("favourite_recategorize", Language::Translate("menu.ped.animation.favourites.option.change_category.label"));
                    if (Keybinds::WasPressedThisFrame("favourite_recategorize"))
                    {
                        dict = prop.modelName;
                        Menu::pendingSubmenu = SUB::SPOONER_SPAWN_PROP_FAVOURITES_CATSELECT;
                    }
                }
            }
        }
    }
}

void Sub_SpawnProp_Favourites_CatSelect()
{
    std::string modelName = dict;
    if (modelName.empty())
    {
        Menu::SetPreviousMenu();
        return;
    }

    std::string currentCategory;
    std::set<std::string> allCategories;
    {
        for (auto& kv : s_favPropCache.byCategory)
        {
            if (kv.first.empty())
                continue;
            allCategories.insert(kv.first);
            for (auto& prop : kv.second)
            {
                if (prop.modelName == modelName)
                    currentCategory = kv.first;
            }
        }
    }

    AddTitle(Language::Translate("menu.spooner.favourites.select_category.title"));

    for (auto& cat : allCategories)
    {
        bool isSelected = (cat == currentCategory);
        bool pressed = false;
        AddTickol(cat, isSelected, pressed, pressed, TICKOL::TICK, TICKOL::NONE);
        if (pressed)
        {
            FavouritesManagement::SetPropCategory(modelName, cat);
            s_favPropCache.needsRebuild = true;
            Menu::SetPreviousMenu();
            return;
        }
    }

    if (!allCategories.empty())
        AddBreak(Language::Translate("menu.spooner.favourites.section.separator.label"));

    bool newCatPressed = false;
    AddOption(Language::Translate("menu.spooner.favourites.option.new_category.label"), newCatPressed);
    if (newCatPressed)
    {
        std::string newCat = Game::InputBox("", 64U, Language::Translate("menu.spooner.favourites.option.new_category_prompt.prompt").c_str(), "");
        if (!newCat.empty())
        {
            FavouritesManagement::SetPropCategory(modelName, newCat);
            s_favPropCache.needsRebuild = true;
            Menu::SetPreviousMenu();
        }
    }

    if (!currentCategory.empty())
    {
        bool removeCatPressed = false;
        AddOption(Language::Translate("menu.spooner.favourites.option.remove_category.label"), removeCatPressed);
        if (removeCatPressed)
        {
            FavouritesManagement::SetPropCategory(modelName, "");
            s_favPropCache.needsRebuild = true;
            Menu::SetPreviousMenu();
        }
    }
}

void Sub_SpawnPed()
{
    bool modelChangerInput = false;
    AddTitle(Language::Translate("menu.spooner.spawn.ped_title.title"));

    AddOption(Language::Translate("menu.spooner.option.favourites.label"), null, nullFunc, SUB::MODELCHANGER_FAVOURITES);
    AddOption(Language::Translate("menu.spooner.spawn.option.search_peds.label"), null, nullFunc, SUB::MODELCHANGER_SEARCH);

    AddOption(Language::Translate("menu.spooner.spawn.option.player.label"), null, nullFunc, SUB::MODELCHANGER_PLAYER);
    AddOption(Language::Translate("menu.spooner.spawn.option.animals.label"), null, nullFunc, SUB::MODELCHANGER_ANIMAL);
    AddOption(Language::Translate("menu.spooner.spawn.option.ambient_females.label"), null, nullFunc, SUB::MODELCHANGER_AMBFEMALES);
    AddOption(Language::Translate("menu.spooner.spawn.option.ambient_males.label"), null, nullFunc, SUB::MODELCHANGER_AMBMALES);
    AddOption(Language::Translate("menu.spooner.spawn.option.cutscene_models.label"), null, nullFunc, SUB::MODELCHANGER_CS);
    AddOption(Language::Translate("menu.spooner.spawn.option.gang_females.label"), null, nullFunc, SUB::MODELCHANGER_GANGFEMALES);
    AddOption(Language::Translate("menu.spooner.spawn.option.gang_males.label"), null, nullFunc, SUB::MODELCHANGER_GANGMALES);
    AddOption(Language::Translate("menu.spooner.spawn.option.story_models.label"), null, nullFunc, SUB::MODELCHANGER_STORY);
    AddOption(Language::Translate("menu.spooner.spawn.option.multiplayer_models.label"), null, nullFunc, SUB::MODELCHANGER_MP);
    AddOption(Language::Translate("menu.spooner.spawn.option.scenario_females.label"), null, nullFunc, SUB::MODELCHANGER_SCENARIOFEMALES);
    AddOption(Language::Translate("menu.spooner.spawn.option.scenario_males.label"), null, nullFunc, SUB::MODELCHANGER_SCENARIOMALES);
    AddOption(Language::Translate("menu.spooner.spawn.option.story_scenario_females.label"), null, nullFunc, SUB::MODELCHANGER_ST_SCENARIOFEMALES);
    AddOption(Language::Translate("menu.spooner.spawn.option.story_scenario_males.label"), null, nullFunc, SUB::MODELCHANGER_ST_SCENARIOMALES);
    AddOption(Language::Translate("menu.spooner.spawn.option.others.label"), null, nullFunc, SUB::MODELCHANGER_OTHERS);
    AddOption(Language::Translate("menu.spooner.spawn.option.input_model.label"), modelChangerInput);

    if (modelChangerInput)
        EntityManagement::InputEntityIntoDb(EntityType::PED);
}

void Sub_SpawnVehicle()
{
    using VehicleSpawner::AddVehicleCategoryOption;
    typedef VehicleSpawner::Indices Indices;

    g_activePedHandle = PLAYER_PED_ID();

    AddTitle(Language::Translate("menu.spooner.spawn.vehicle_title.title"));
    AddOption(Language::Translate("menu.spooner.option.favourites.label"), null, nullFunc, SUB::SPAWNVEHICLE_FAVOURITES);
    AddOption(Language::Translate("menu.spooner.spawn.option.search_vehicles.label"), null, nullFunc, SUB::SPAWNVEHICLE_SEARCH);

    AddBreak(Language::Translate("menu.spooner.spawn.section.cars.label"));
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.open_wheel.label"), Indices::OPENWHEEL);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.super.label"), Indices::SUPER);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.sports.label"), Indices::SPORT);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.sports_classics.label"), Indices::SPORTSCLASSIC);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.coupes.label"), Indices::COUPE);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.muscle.label"), Indices::MUSCLE);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.offroad.label"), Indices::OFFROAD);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.suvs.label"), Indices::SUV);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.sedans.label"), Indices::SEDAN);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.compacts.label"), Indices::COMPACT);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.drift.label"), Indices::DRIFT);

    AddBreak(Language::Translate("menu.spooner.spawn.section.industrial.label"));
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.vans.label"), Indices::VAN);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.services.label"), Indices::SERVICE);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.industrial_category.label"), Indices::INDUSTRIAL);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.commercial.label"), Indices::COMMERCIAL);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.utility.label"), Indices::UTILITY);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.trains.label"), Indices::TRAIN);

    AddBreak(Language::Translate("menu.spooner.spawn.section.others_section.label"));
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.emergency.label"), Indices::EMERGENCY);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.military.label"), Indices::MILITARY);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.motorcycles.label"), Indices::MOTORCYCLE);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.bicycles.label"), Indices::BICYCLE);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.planes.label"), Indices::PLANE);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.helicopters.label"), Indices::HELICOPTER);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.boats.label"), Indices::BOAT);
    AddVehicleCategoryOption(Language::Translate("menu.spooner.spawn.option.others.label"), Indices::OTHER);
}
} // namespace Spooner::Submenus

void EntityAlphaLevelSub_()
{
    GTAentity entity = Spooner::Submenus::s_selectedEntityTarget;
    auto alphaLevel = entity.GetAlpha();

    AddTitle(Language::Translate("menu.spooner.opacity.title"));

    bool bReset = false;
    AddOption(Language::Translate("menu.spooner.opacity.option.reset.label"), bReset);
    if (bReset)
    {
        entity.RequestControl();
        entity.ResetAlpha();
    }

    std::array<BYTE, 5> alphaLevels = {0, 50, 89, 160, 255};

    for (auto& lvl : alphaLevels)
    {
        bool bAlphaLevelPressed = false;
        AddTickol(std::to_string((int)(((float)(lvl) / 255.0f) * 100)) + "%", alphaLevel == lvl, bAlphaLevelPressed, null);
        if (bAlphaLevelPressed)
        {
            entity.RequestControl();
            entity.SetAlpha(lvl);
        }
    }

    AddBreak(Language::Translate("menu.spooner.opacity.section.manual.label"));
    bool bInputPressed = false, bPlusPressed = false, bMinusPressed = false;
    AddNumber(Language::Translate("menu.spooner.opacity.option.level.label"), alphaLevel, 0, bInputPressed, bPlusPressed, bMinusPressed);
    if (bPlusPressed)
    {
        if (alphaLevel < 255)
        {
            alphaLevel++;
            entity.RequestControl();
            entity.SetAlpha(alphaLevel);
        }
    }
    if (bMinusPressed)
    {
        if (alphaLevel > 0)
        {
            alphaLevel--;
            entity.RequestControl();
            entity.SetAlpha(alphaLevel);
        }
    }
    if (bInputPressed)
    {
        std::string inputStr = Game::InputBox(std::string(), 4U, Language::Translate("menu.spooner.opacity.option.level_prompt.prompt").c_str(), std::to_string(alphaLevel));
        if (inputStr.length() > 0)
        {
            try
            {
                alphaLevel = stoi(inputStr);
                if (alphaLevel < 0 || alphaLevel > 255)
                    throw;
                entity.RequestControl();
                entity.SetAlpha(alphaLevel);
            }
            catch (...)
            {
                Game::Print::PrintErrorInvalidInput(inputStr);
            }
        }
        //OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::AlphaLevelMenuInput, std::string(), 3U, "Enter an alpha level (0-255):", std::to_string(alphaLevel));
        //OnscreenKeyboard::State::arg1._int = entity.GetHandle();
        //OnscreenKeyboard::State::arg2._int = alphaLevel;
    }
}

} // namespace sub

#include "..\..\Menu\submenu_switch.h"
#include "..\..\Menu\submenu_enum.h"
REGISTER_SUBMENU(ENTITYALPHALEVEL, sub::EntityAlphaLevelSub_)
REGISTER_SUBMENU(SPOONER_MAIN, sub::Spooner::Submenus::Sub_SpoonerMain)
REGISTER_SUBMENU(SPOONER_SPAWN_CATEGORIES, sub::Spooner::Submenus::Sub_SpawnCategories)
REGISTER_SUBMENU(SPOONER_SPAWN_PROP, sub::Spooner::Submenus::Sub_SpawnProp)
REGISTER_SUBMENU(SPOONER_SPAWN_PROP_FAVOURITES, sub::Spooner::Submenus::Sub_SpawnProp_Favourites)
REGISTER_SUBMENU(SPOONER_SPAWN_PROP_FAVOURITES_CATSELECT, sub::Spooner::Submenus::Sub_SpawnProp_Favourites_CatSelect)
REGISTER_SUBMENU(SPOONER_SPAWN_PED, sub::Spooner::Submenus::Sub_SpawnPed)
REGISTER_SUBMENU(SPOONER_SPAWN_VEHICLE, sub::Spooner::Submenus::Sub_SpawnVehicle)
REGISTER_SUBMENU(SPOONER_MANAGEMARKERS, sub::Spooner::Submenus::Sub_ManageMarkers)
REGISTER_SUBMENU(SPOONER_MANAGEMARKERS_REMOVAL, sub::Spooner::Submenus::Sub_ManageMarkers_Removal)
REGISTER_SUBMENU(SPOONER_MANAGEMARKERS_INMARKER, sub::Spooner::Submenus::Sub_ManageMarkers_InMarker)
REGISTER_SUBMENU(SPOONER_MANAGEMARKERS_INMARKER_DEST2MARKER, sub::Spooner::Submenus::Sub_ManageMarkers_InMarker_Dest2Marker)
REGISTER_SUBMENU(SPOONER_MANAGEMARKERS_INMARKER_ATTACH, sub::Spooner::Submenus::Sub_ManageMarkers_InMarker_Attach)
REGISTER_SUBMENU(SPOONER_MANAGELIGHTS, sub::Spooner::Submenus::Sub_ManageLights)
REGISTER_SUBMENU(SPOONER_MANAGELIGHTS_REMOVAL, sub::Spooner::Submenus::Sub_ManageLights_Removal)
REGISTER_SUBMENU(SPOONER_MANAGELIGHTS_INLIGHT, sub::Spooner::Submenus::Sub_ManageLights_InLight)
REGISTER_SUBMENU(SPOONER_MANAGELIGHTS_PRESETS, sub::Spooner::Submenus::Sub_ManageLights_Presets)
REGISTER_SUBMENU(SPOONER_MANAGELIGHTS_COLOUR, sub::Spooner::Submenus::Sub_ManageLights_Colour)
REGISTER_SUBMENU(SPOONER_MANAGEDB, sub::Spooner::Submenus::Sub_ManageEntities)
REGISTER_SUBMENU(SPOONER_MANAGEDB_REMOVAL, sub::Spooner::Submenus::Sub_ManageEntities_Removal)
REGISTER_SUBMENU(SPOONER_SAVEFILES, sub::Spooner::Submenus::Sub_SaveFiles)
REGISTER_SUBMENU(SPOONER_SAVEFILES_LOAD, sub::Spooner::Submenus::Sub_SaveFiles_Load)
REGISTER_SUBMENU(SPOONER_SAVEFILES_LOAD_LEGACYSP00N, sub::Spooner::Submenus::Sub_SaveFiles_Load_LegacySP00N)
REGISTER_SUBMENU(SPOONER_AUTOSAVE, sub::Spooner::Submenus::Sub_AutoSave)
REGISTER_SUBMENU(SPOONER_VECTOR3_MANUALEDITING, sub::Spooner::Submenus::Sub_Vector3_ManualEditing)
REGISTER_SUBMENU(SPOONER_MULTISELECT, sub::Spooner::Submenus::Sub_MultiSelect)
REGISTER_SUBMENU(SPOONER_JOBIMPORTER, sub::Spooner::Submenus::Sub_JobImporter)
REGISTER_SUBMENU(SPOONER_JOBBROWSER, sub::Spooner::Submenus::Sub_JobBrowser)
REGISTER_SUBMENU(SPOONER_JOBBROWSER_MYCONTENT, sub::Spooner::Submenus::Sub_JobBrowser)
REGISTER_SUBMENU(SPOONER_JOBBROWSER_FRIENDCONTENT, sub::Spooner::Submenus::Sub_JobBrowser)
REGISTER_SUBMENU(SPOONER_JOBBROWSER_MOSTRECENT, sub::Spooner::Submenus::Sub_JobBrowser)
REGISTER_SUBMENU(SPOONER_JOBBROWSER_TOPRATED, sub::Spooner::Submenus::Sub_JobBrowser)
REGISTER_SUBMENU(SPOONER_JOBBROWSER_CREWCONTENT, sub::Spooner::Submenus::Sub_JobBrowser)
REGISTER_SUBMENU(SPOONER_JOBBROWSER_INFO, sub::Spooner::Submenus::Sub_JobBrowser_Info)
REGISTER_SUBMENU(SPOONER_SETTINGS, sub::Spooner::Submenus::Sub_Settings)
REGISTER_SUBMENU(SPOONER_SELECTEDENTITYOPS, sub::Spooner::Submenus::Sub_SelectedEntityOps)
REGISTER_SUBMENU(SPOONER_PEDOPS, sub::Spooner::Submenus::Sub_PedOps)
REGISTER_SUBMENU(SPOONER_PEDOPS_WEAPON, sub::Spooner::Submenus::Sub_PedOps_Weapon)
REGISTER_SUBMENU(SPOONER_PEDOPS_WEAPON_INCATEGORY, sub::Spooner::Submenus::Sub_PedOps_Weapon_InCategory)
REGISTER_SUBMENU(SPOONER_ATTACHMENTOPS, sub::Spooner::Submenus::Sub_AttachmentOps)
REGISTER_SUBMENU(SPOONER_ATTACHMENTOPS_ATTACHTO, sub::Spooner::Submenus::Sub_AttachmentOps_AttachTo)
REGISTER_SUBMENU(SPOONER_ATTACHMENTOPS_SELECTBONE, sub::Spooner::Submenus::Sub_AttachmentOps_SelectBone)
REGISTER_SUBMENU(SPOONER_MANUALEDITING, sub::Spooner::Submenus::Sub_ManualEditing)
REGISTER_SUBMENU(SPOONER_MANUALEDITING_SNAP, sub::Spooner::Submenus::Sub_Snapping)
REGISTER_SUBMENU(OBJECTSPAWNER_SEARCH, sub::Spooner::Submenus::ObjectSpawnerSearchMenu)
REGISTER_SUBMENU(SPOONER_BLIPS, sub::Spooner::Submenus::Sub_Blip_Management)
REGISTER_SUBMENU(SPOONER_BLIPS_ADD_RADIAL, sub::Spooner::Submenus::Sub_Blip_Radial)
REGISTER_SUBMENU(SPOONER_BLIPS_ADD_ENTITY, sub::Spooner::Submenus::Sub_Blip_Entity)
REGISTER_SUBMENU(SPOONER_BLIPS_ADD_COORD, sub::Spooner::Submenus::Sub_Blip_Coord)
REGISTER_SUBMENU(SPOONER_BLIPS_RADIALINBLIP, sub::Spooner::Submenus::Sub_Blip_RadialInBlip)
REGISTER_SUBMENU(SPOONER_BLIPS_COORDINBLIP, sub::Spooner::Submenus::Sub_Blip_CoordInBlip)
REGISTER_SUBMENU(SPOONER_BLIPS_ENTITYINBLIP, sub::Spooner::Submenus::Sub_Blip_EntityInBlip)
REGISTER_SUBMENU(SPOONER_BLIPS_ENTITY_SELECT, sub::Spooner::Submenus::Sub_Blip_Entity_Select)
REGISTER_SUBMENU(SPOONER_BLIPS_ATTACH, sub::Spooner::Submenus::Sub_Blip_Attach)
REGISTER_SUBMENU(SPOONER_BLIPS_ICONS, sub::Spooner::Submenus::Sub_Blip_Icons)
