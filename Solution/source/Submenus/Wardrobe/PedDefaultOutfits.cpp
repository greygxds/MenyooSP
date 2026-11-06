/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#include "..\..\macros.h"
#include "..\..\Menu\Language.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Menu\Routine.h"
#include "..\..\Natives\natives2.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\Game.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Util\FileLogger.h"
#include "..\..\Util\StringManip.h"

#include <cstring>
#include <string>
#include <vector>

#include "..\..\Menu\submenu_switch.h"
#include "..\..\Menu\submenu_enum.h"

namespace sub
{
namespace
{
struct DefaultPedOutfit
{
    Hash hash = 0;
    int propCount = 0;
    int componentCount = 0;
    std::string textLabel;
    std::string displayName;
};

std::vector<DefaultPedOutfit> defaultPedOutfitCache;
int cachedCharacterType = -1;
std::string outfitSearchText;

int GetCharacterType(Hash modelHash)
{
    // Only Freemode Characters seem to work.  It should work for Story Mode Characters but nothing comes up.  They don't have many anyway.
    if (modelHash == GET_HASH_KEY("Player_Zero"))
        return 0;
    if (modelHash == GET_HASH_KEY("Player_One"))
        return 1;
    if (modelHash == GET_HASH_KEY("Player_Two"))
        return 2;
    if (modelHash == GET_HASH_KEY("MP_M_Freemode_01"))
        return 3;
    if (modelHash == GET_HASH_KEY("MP_F_Freemode_01"))
        return 4;
    return -1;
}

std::string ResolveDisplayName(const std::string& textLabel)
{
    if (textLabel.empty())
        return textLabel;

    const char* resolvedName = DOES_TEXT_LABEL_EXIST((char*)textLabel.c_str()) ? GET_FILENAME_FOR_AUDIO_CONVERSATION((char*)textLabel.c_str()) : nullptr;
    if (resolvedName && std::strlen(resolvedName) > 0 && std::strcmp(resolvedName, "NULL") != 0)
        return resolvedName;

    return textLabel;
}

DefaultPedOutfit ReadDefaultPedOutfit(int outfitIndex)
{
    int outfitData[32] = {};
    FILES::GET_SHOP_PED_QUERY_OUTFIT(outfitIndex, (Any*)outfitData);

    DefaultPedOutfit outfit;
    outfit.hash = static_cast<Hash>(outfitData[2]);
    outfit.propCount = outfitData[6];
    outfit.componentCount = outfitData[8];

    // The native stores the text label at byte 56 in this 128-byte result.
    const char* textLabel = reinterpret_cast<const char*>(outfitData) + 56;
    outfit.textLabel = textLabel;
    outfit.displayName = ResolveDisplayName(outfit.textLabel);
    return outfit;
}

std::vector<DefaultPedOutfit> LoadDefaultPedOutfits(int characterType)
{
    std::vector<DefaultPedOutfit> outfits;
    const int outfitCount = FILES::SETUP_SHOP_PED_OUTFIT_QUERY(characterType, false);
    for (int outfitIndex = 0; outfitIndex < outfitCount; ++outfitIndex)
        outfits.push_back(ReadDefaultPedOutfit(outfitIndex));
    return outfits;
}

void UpdateDefaultPedOutfitCache(int characterType)
{
    if (characterType != cachedCharacterType)
    {
        defaultPedOutfitCache.clear();
        cachedCharacterType = characterType;
        outfitSearchText.clear();
    }

    if (defaultPedOutfitCache.empty() && characterType >= 0)
        defaultPedOutfitCache = LoadDefaultPedOutfits(characterType);
}

void DrawOutfitSearchOption()
{
    bool searchPressed = false;
    AddOption(outfitSearchText.empty() ? Language::Translate("components.search") : outfitSearchText, searchPressed, nullFunc, -1, true);
    if (!searchPressed)
        return;

    outfitSearchText = Game::InputBox(outfitSearchText, 126U, Language::Translate("components.search.prompt").c_str(), boost::to_lower_copy(outfitSearchText));
    boost::to_lower(outfitSearchText);
}

bool MatchesOutfitSearch(const DefaultPedOutfit& outfit)
{
    if (outfitSearchText.empty())
        return true;

    const std::string lowerDisplayName = boost::to_lower_copy(outfit.displayName);
    const std::string lowerTextLabel = boost::to_lower_copy(outfit.textLabel);
    return lowerDisplayName.find(outfitSearchText) != std::string::npos || lowerTextLabel.find(outfitSearchText) != std::string::npos;
}

void ClearOutfitVariations(GTAped ped)
{
    SET_PED_DEFAULT_COMPONENT_VARIATION(ped.Handle());

    for (int propIndex = 0; propIndex < PV_PROP_MAX; ++propIndex)
        ped.SetPropVariation(propIndex, -1, 0, false);

    constexpr int componentIndicesToClear[] = {1, 5, 7, 8, 9, 10};
    for (const int componentIndex : componentIndicesToClear)
    {
        if (ped.IsComponentVariationValid(componentIndex, 0, 0))
            ped.SetComponentVariation(componentIndex, 0, 0);
    }
}

void ApplyDefaultPedOutfitComponents(GTAped ped, const DefaultPedOutfit& outfit)
{
    for (int componentIndex = 0; componentIndex < outfit.componentCount; ++componentIndex)
    {
        int variantData[8] = {};
        FILES::GET_SHOP_PED_OUTFIT_COMPONENT_VARIANT(outfit.hash, componentIndex, (Any*)variantData);
        const Hash componentHash = static_cast<Hash>(variantData[0]);
        const int componentType = variantData[4];

        int componentData[20] = {};
        FILES::GET_SHOP_PED_COMPONENT(componentHash, (Any*)componentData);
        const int drawableIndex = componentData[6];
        const int textureIndex = componentData[8];
        if (ped.IsComponentVariationValid(componentType, drawableIndex, textureIndex))
            ped.SetComponentVariation(componentType, drawableIndex, textureIndex);
    }
}

void ApplyDefaultPedOutfitProps(GTAped ped, const DefaultPedOutfit& outfit)
{
    for (int propIndex = 0; propIndex < outfit.propCount; ++propIndex)
    {
        int variantData[8] = {};
        FILES::GET_SHOP_PED_OUTFIT_PROP_VARIANT(outfit.hash, propIndex, (Any*)variantData);
        const Hash propHash = static_cast<Hash>(variantData[0]);
        const int anchorPoint = variantData[4];

        int propData[20] = {};
        FILES::GET_SHOP_PED_PROP(propHash, (Any*)propData);
        const int drawableIndex = propData[6];
        const int textureIndex = propData[8];
        if (ped.IsPropVariationValid(anchorPoint, drawableIndex, textureIndex))
            ped.SetPropVariation(anchorPoint, drawableIndex, textureIndex, true);
    }
}

void ApplyDefaultPedOutfit(GTAped ped, const DefaultPedOutfit& outfit)
{
    ped.RequestControl(200);
    ClearOutfitVariations(ped);

    addlog(
        ige::LogType::LOG_DEBUG,
        "Applied Default Outfit: " + outfit.displayName + " | Hash: " + std::to_string(outfit.hash) + " | Comps: " + std::to_string(outfit.componentCount) +
            " | Props: " + std::to_string(outfit.propCount)
    );

    ApplyDefaultPedOutfitComponents(ped, outfit);
    ApplyDefaultPedOutfitProps(ped, outfit);
}
} // namespace

void PedDefaultOutfitsMenu()
{
    GTAped ped = g_activePedHandle;
    const int characterType = GetCharacterType(ped.Model().hash);

    AddTitle(Language::Translate("components.outfits.defaultTitle"));
    UpdateDefaultPedOutfitCache(characterType);

    if (characterType < 0)
    {
        bool optionPressed = false;
        AddOption(Language::Translate("components.outfits.none"), optionPressed);
        return;
    }

    DrawOutfitSearchOption();
    for (const DefaultPedOutfit& outfit : defaultPedOutfitCache)
    {
        if (!MatchesOutfitSearch(outfit))
            continue;

        bool outfitPressed = false;
        AddOption(outfit.displayName, outfitPressed);
        if (outfitPressed)
            ApplyDefaultPedOutfit(ped, outfit);
    }
}
} // namespace sub

REGISTER_SUBMENU(PED_DEFAULT_OUTFITS, sub::PedDefaultOutfitsMenu)
