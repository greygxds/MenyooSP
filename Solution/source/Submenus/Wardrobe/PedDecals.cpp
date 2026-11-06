/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#include "PedDecals.h"
#include "..\..\Menu\Keybinds.h"
#include "..\..\Menu\Language.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Menu\Routine.h"
#include "..\..\Menu\submenu_enum.h"
#include "..\..\Menu\submenu_switch.h"
#include "..\..\Natives\natives2.h"
#include "..\..\Scripting\Game.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Util\ExePath.h"
#include "..\..\Util\FileLogger.h"
#include "..\..\Util\StringManip.h"
#include "..\..\macros.h"

#include <pugixml\src\pugixml.hpp>

#include <map>
#include <string>
#include <vector>

namespace sub
{
namespace PedDecals
{
namespace
{
struct PedDecalDefinition
{
    Hash collectionHash;
    Hash overlayHash;
    std::string caption;

    void ApplyToPed(GTAped ped) const
    {
        if (!ped.Exists())
        {
            ped.ForgetTrackedDecorations();
            return;
        }

        ped.RequestControl(200);
        ped.ApplyDecoration(collectionHash, overlayHash);
    }

    void RemoveFromPed(GTAped ped) const
    {
        if (!ped.Exists())
        {
            ped.ForgetTrackedDecorations();
            return;
        }

        ped.RequestControl(200);
        ped.RemoveDecoration(collectionHash, overlayHash);
    }
};

using DecalsByZoneName = std::map<std::string, std::vector<PedDecalDefinition>>;
using DecalsByTypeName = std::map<std::string, DecalsByZoneName>;
using DecalCatalogByPedModelHash = std::map<Hash, DecalsByTypeName>;

struct DecalMenuSelection
{
    Hash pedModelHash = 0;
    std::string typeName;
    std::string zoneName;
};

struct DecalSearchResult
{
    std::string zoneName;
    const PedDecalDefinition* decalDefinition = nullptr;
};

struct DecalSearchState
{
    std::vector<DecalSearchResult> results;
    bool resultsNeedRebuild = true;
    std::string searchTerm;
    std::string lastBuiltSearchTerm;
    Hash lastBuiltPedModelHash = 0;
    std::string lastBuiltTypeName;
};

struct DecalPreviewState
{
    bool isEnabled = false;
    const PedDecalDefinition* previewedDecal = nullptr;
};

DecalCatalogByPedModelHash decalCatalogByPedModelHash;
DecalMenuSelection decalMenuSelection;
DecalSearchState decalSearchState;
DecalPreviewState decalPreviewState;

const DecalsByZoneName* FindSelectedDecalType()
{
    const auto pedModelIt = decalCatalogByPedModelHash.find(decalMenuSelection.pedModelHash);
    if (pedModelIt == decalCatalogByPedModelHash.end())
        return nullptr;

    const auto decalTypeIt = pedModelIt->second.find(decalMenuSelection.typeName);
    if (decalTypeIt == pedModelIt->second.end())
        return nullptr;

    return &decalTypeIt->second;
}

const std::vector<PedDecalDefinition>* FindSelectedDecalZone()
{
    const auto* decalsByZoneName = FindSelectedDecalType();
    if (decalsByZoneName == nullptr)
        return nullptr;

    const auto decalZoneIt = decalsByZoneName->find(decalMenuSelection.zoneName);
    if (decalZoneIt == decalsByZoneName->end())
        return nullptr;

    return &decalZoneIt->second;
}

void ClearPreviewedDecal()
{
    if (decalPreviewState.previewedDecal && DOES_ENTITY_EXIST(g_activePedHandle))
    {
        GTAped activePed = g_activePedHandle;
        decalPreviewState.previewedDecal->RemoveFromPed(activePed);
        decalPreviewState.previewedDecal = nullptr;
    }
}

void DrawPreviewModeToggle()
{
    Keybinds::AddBindIB("menu_action", decalPreviewState.isEnabled, Language::Translate("components.decals.previewOn"), Language::Translate("components.decals.previewOff"));
    if (Keybinds::WasPressedThisFrame("menu_action"))
    {
        decalPreviewState.isEnabled = !decalPreviewState.isEnabled;
        if (!decalPreviewState.isEnabled)
            ClearPreviewedDecal();
    }
}

void DrawDecalOption(const std::string& label, const PedDecalDefinition& decalDefinition, GTAped targetPed)
{
    const bool isOptionHovered = (*Menu::activeOptionIndex == Menu::currentOptionCount + 1);
    bool applyDecalPressed = false;
    bool removeDecalPressed = false;
    const bool isDecalApplied = targetPed.HasTrackedDecoration(decalDefinition.collectionHash, decalDefinition.overlayHash);

    AddTickol(label, isDecalApplied, applyDecalPressed, removeDecalPressed, TICKOL::TATTOOTHING);

    if (decalPreviewState.isEnabled && isOptionHovered && decalPreviewState.previewedDecal != &decalDefinition)
    {
        ClearPreviewedDecal();
        if (!isDecalApplied)
        {
            decalDefinition.ApplyToPed(targetPed);
            decalPreviewState.previewedDecal = &decalDefinition;
        }
    }

    if (applyDecalPressed)
    {
        decalDefinition.ApplyToPed(targetPed);
    }
    else if (removeDecalPressed && decalPreviewState.previewedDecal == &decalDefinition)
    {
        ClearPreviewedDecal();
        decalDefinition.ApplyToPed(targetPed);
    }
    else if (removeDecalPressed)
    {
        decalDefinition.RemoveFromPed(targetPed);
    }
}

void RebuildSearchResults(const DecalsByZoneName& decalsByZoneName, const std::string& searchTerm)
{
    decalSearchState.results.clear();
    const std::string uppercaseSearchTerm = boost::to_upper_copy(searchTerm);

    for (const auto& [zoneName, decals] : decalsByZoneName)
    {
        for (const auto& decalDefinition : decals)
        {
            const std::string uppercaseCaption = boost::to_upper_copy(decalDefinition.caption);
            if (!uppercaseSearchTerm.empty() && uppercaseCaption.find(uppercaseSearchTerm) == std::string::npos)
                continue;

            decalSearchState.results.push_back({zoneName, &decalDefinition});
        }
    }
}

bool SearchResultsNeedRebuild()
{
    return decalSearchState.resultsNeedRebuild || decalSearchState.searchTerm != decalSearchState.lastBuiltSearchTerm || decalMenuSelection.pedModelHash != decalSearchState.lastBuiltPedModelHash ||
           decalMenuSelection.typeName != decalSearchState.lastBuiltTypeName;
}

void DrawDecalTypesMenu()
{
    GTAped activePed = g_activePedHandle;
    const Hash pedModelHash = activePed.Model().hash;
    const auto pedModelIt = decalCatalogByPedModelHash.find(pedModelHash);
    if (pedModelIt == decalCatalogByPedModelHash.end())
    {
        Menu::SetPreviousMenu();
        return;
    }

    AddTitle(Language::Translate("components.decals.title"));

    for (const auto& decalTypeEntry : pedModelIt->second)
    {
        const auto& decalTypeName = decalTypeEntry.first;
        bool decalTypeSelected = false;
        AddOption(decalTypeName, decalTypeSelected, nullFunc, SUB::PED_DECALS_ZONES);
        if (decalTypeSelected)
        {
            decalMenuSelection.pedModelHash = pedModelHash;
            decalMenuSelection.typeName = decalTypeName;
            decalMenuSelection.zoneName.clear();
            decalSearchState.resultsNeedRebuild = true;
        }
    }

    bool clearAllDecalsPressed = false;
    AddTickol(Language::Translate("components.decals.clearAll"), true, clearAllDecalsPressed, clearAllDecalsPressed, TICKOL::CROSS);
    if (clearAllDecalsPressed)
    {
        activePed.RequestControl(600);
        activePed.ClearDecorations();
    }
}

void DrawDecalZonesMenu()
{
    const auto* decalsByZoneName = FindSelectedDecalType();
    if (decalsByZoneName == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    AddTitle(decalMenuSelection.typeName);

    bool searchMenuPressed = false;
    AddOption(Language::Translate("components.search.button"), searchMenuPressed, nullFunc, SUB::PED_DECALS_ZONE_SEARCH, true);

    AddBreak(Language::Translate("components.decals.categories"));

    for (const auto& decalZoneEntry : *decalsByZoneName)
    {
        const auto& decalZoneName = decalZoneEntry.first;
        bool decalZoneSelected = false;
        AddOption(decalZoneName, decalZoneSelected, nullFunc, SUB::PED_DECALS_IN_ZONE);
        if (decalZoneSelected)
            decalMenuSelection.zoneName = decalZoneName;
    }
}

void DrawDecalSearchMenu()
{
    GTAped activePed = g_activePedHandle;
    const auto* decalsByZoneName = FindSelectedDecalType();
    if (decalsByZoneName == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    if (Menu::OnSubBack == nullptr)
    {
        Menu::OnSubBack = []
        {
            ClearPreviewedDecal();
            decalSearchState.searchTerm.clear();
        };
    }

    AddTitle(Language::Translate("components.search.title"));

    bool searchInputPressed = false;
    AddOption(decalSearchState.searchTerm.empty() ? Language::Translate("components.search.button") : ("~b~" + decalSearchState.searchTerm + "~s~"), searchInputPressed, nullFunc, -1, true);
    if (searchInputPressed)
    {
        decalSearchState.searchTerm =
            Game::InputBox(decalSearchState.searchTerm, 64U, Language::Translate("components.decals.searchPrompt").c_str(), boost::to_lower_copy(decalSearchState.searchTerm));
        boost::to_upper(decalSearchState.searchTerm);
    }

    if (SearchResultsNeedRebuild())
    {
        RebuildSearchResults(*decalsByZoneName, decalSearchState.searchTerm);
        decalSearchState.lastBuiltSearchTerm = decalSearchState.searchTerm;
        decalSearchState.lastBuiltPedModelHash = decalMenuSelection.pedModelHash;
        decalSearchState.lastBuiltTypeName = decalMenuSelection.typeName;
        decalSearchState.resultsNeedRebuild = false;
    }

    AddBreak(Language::Format("components.decals.results", {{"count", decalSearchState.results.size()}}));

    for (const auto& searchResult : decalSearchState.results)
    {
        const std::string label = "~c~" + searchResult.zoneName + " / ~s~" + searchResult.decalDefinition->caption;
        DrawDecalOption(label, *searchResult.decalDefinition, activePed);
    }

    DrawPreviewModeToggle();
}

void DrawDecalsInZoneMenu()
{
    GTAped activePed = g_activePedHandle;
    const auto* decalsInZone = FindSelectedDecalZone();
    if (decalsInZone == nullptr)
    {
        Menu::SetPreviousMenu();
        return;
    }

    if (Menu::OnSubBack == nullptr)
        Menu::OnSubBack = [] { ClearPreviewedDecal(); };

    AddTitle(decalMenuSelection.zoneName);

    for (const auto& decalDefinition : *decalsInZone)
        DrawDecalOption(decalDefinition.caption, decalDefinition, activePed);

    DrawPreviewModeToggle();
}
} // namespace

void LoadDecalCatalog()
{
    decalCatalogByPedModelHash.clear();
    decalMenuSelection = {};
    decalSearchState = {};
    decalPreviewState = {};

    pugi::xml_document document;
    const std::string catalogPath = GetPathffA(Pathff::Main, true) + "PedDecalOverlays.xml";
    if (document.load_file(catalogPath.c_str()).status != pugi::status_ok)
    {
        addlog(ige::LogType::LOG_ERROR, "Unable to open PedDecalOverlays.xml");
        return;
    }

    const auto rootNode = document.document_element();
    for (auto pedNode = rootNode.child("Ped"); pedNode; pedNode = pedNode.next_sibling("Ped"))
    {
        const Hash pedModelHash = pedNode.attribute("hash").as_uint();
        auto& decalsByTypeName = decalCatalogByPedModelHash[pedModelHash];
        for (auto decalTypeNode = pedNode.first_child(); decalTypeNode; decalTypeNode = decalTypeNode.next_sibling())
        {
            auto& decalsByZoneName = decalsByTypeName[decalTypeNode.name()];
            for (auto decalZoneNode = decalTypeNode.first_child(); decalZoneNode; decalZoneNode = decalZoneNode.next_sibling())
            {
                auto& decalsInZone = decalsByZoneName[decalZoneNode.name()];
                for (auto overlayNode = decalZoneNode.child("OVERLAY"); overlayNode; overlayNode = overlayNode.next_sibling("OVERLAY"))
                {
                    PedDecalDefinition decalDefinition;
                    decalDefinition.collectionHash = GET_HASH_KEY(overlayNode.attribute("collection").as_string());
                    decalDefinition.overlayHash = GET_HASH_KEY(overlayNode.attribute("name").as_string());
                    decalDefinition.caption = overlayNode.attribute("caption").as_string();
                    decalsInZone.push_back(decalDefinition);
                }
            }
        }
    }
}

std::string FindDecalCaption(Hash collectionHash, Hash overlayHash)
{
    for (const auto& pedModelEntry : decalCatalogByPedModelHash)
    {
        for (const auto& decalTypeEntry : pedModelEntry.second)
        {
            for (const auto& decalZoneEntry : decalTypeEntry.second)
            {
                for (const auto& decalDefinition : decalZoneEntry.second)
                {
                    if (decalDefinition.collectionHash == collectionHash && decalDefinition.overlayHash == overlayHash)
                        return decalDefinition.caption;
                }
            }
        }
    }
    return {};
}

void OpenDecalsMenu()
{
    GTAped activePed = g_activePedHandle;
    const auto pedModelIt = decalCatalogByPedModelHash.find(activePed.Model().hash);
    if (pedModelIt != decalCatalogByPedModelHash.end())
    {
        Menu::pendingSubmenu = SUB::PED_DECALS_TYPES;
        return;
    }

    Game::Print::ShowNotification(Language::Translate("components.error"), Language::Translate("components.decals.none"));
}
} // namespace PedDecals
} // namespace sub

REGISTER_SUBMENU(PED_DECALS_TYPES, sub::PedDecals::DrawDecalTypesMenu)
REGISTER_SUBMENU(PED_DECALS_ZONES, sub::PedDecals::DrawDecalZonesMenu)
REGISTER_SUBMENU(PED_DECALS_IN_ZONE, sub::PedDecals::DrawDecalsInZoneMenu)
REGISTER_SUBMENU(PED_DECALS_ZONE_SEARCH, sub::PedDecals::DrawDecalSearchMenu)
