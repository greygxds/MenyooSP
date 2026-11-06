/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "PedHeadFeatures.h"
#include "WardrobeCamera.h"
#include "..\..\Menu\Language.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Menu\Keybinds.h"
#include "..\..\Menu\Routine.h"
#include "..\..\Menu\StatsPanel.h"
#include "..\..\Natives\natives2.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Scripting\ModelNames.h"
#include "..\..\Scripting\Game.h"
#include "..\..\Util\FileLogger.h"
#include "..\..\Util\StringManip.h"
#include "..\..\macros.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sub
{
static int selectedOverlayIndex = 0;

// Wraps an int value around [minVal, maxVal) on increment/decrement
static int cycleInt(int current, bool increment, int minVal, int maxVal)
{
    if (increment)
        return (current < maxVal) ? current + 1 : minVal;
    else
        return (current > minVal) ? current - 1 : maxVal;
}

// Wraps a float value around [minVal, maxVal] with a given step
static float cycleFloat(float current, bool increment, float minVal, float maxVal, float step)
{
    if (increment)
        return (current < maxVal) ? current + step : current;
    else
        return (current > minVal) ? current - step : current;
}

// Head features (freemode m/f)

namespace PedHeadFeatures_catind
{

std::map<Ped, PedHeadFeatureSelection> pedHeadFeaturesByPed;
std::map<Ped, PedHeadFeatureSelection>::mapped_type* selectedPedHeadFeatures;

#pragma region arrays

struct HeadOverlayCaptions
{
    std::string_view localizationId;
    std::string title;
    std::vector<std::string> variations;
};

static const std::array<HeadOverlayCaptions, 13> vCaptions_headOverlays{{
    {"skin_rash", "FACE_F_SUND", {"Uneven", "Sandpaper", "Patchy", "Rough", "Leathery", "Textured", "Coarse", "Rugged", "Creased", "Cracked", "Gritty"}},
    {"beard",
     "FACE_F_BEARD",
     {"HAIR_BEARD1",
      "HAIR_BEARD2",
      "HAIR_BEARD3",
      "HAIR_BEARD4",
      "HAIR_BEARD5",
      "HAIR_BEARD6",
      "HAIR_BEARD7",
      "HAIR_BEARD8",
      "HAIR_BEARD9",
      "HAIR_BEARD10",
      "HAIR_BEARD11",
      "HAIR_BEARD12",
      "HAIR_BEARD13",
      "HAIR_BEARD14",
      "HAIR_BEARD15",
      "HAIR_BEARD16",
      "HAIR_BEARD17",
      "HAIR_BEARD18",
      "HAIR_BEARD19"}}, // Beard HAIR_OPTION_0
    {"eyebrows", "FACE_F_EYEBR", {"CC_EYEBRW_0",  "CC_EYEBRW_1",  "CC_EYEBRW_2",  "CC_EYEBRW_3",  "CC_EYEBRW_4",  "CC_EYEBRW_5",  "CC_EYEBRW_6",  "CC_EYEBRW_7",  "CC_EYEBRW_8",
                                  "CC_EYEBRW_9",  "CC_EYEBRW_10", "CC_EYEBRW_11", "CC_EYEBRW_12", "CC_EYEBRW_13", "CC_EYEBRW_14", "CC_EYEBRW_15", "CC_EYEBRW_16", "CC_EYEBRW_17",
                                  "CC_EYEBRW_18", "CC_EYEBRW_19", "CC_EYEBRW_20", "CC_EYEBRW_21", "CC_EYEBRW_22", "CC_EYEBRW_23", "CC_EYEBRW_24", "CC_EYEBRW_25", "CC_EYEBRW_26",
                                  "CC_EYEBRW_27", "CC_EYEBRW_28", "CC_EYEBRW_29", "CC_EYEBRW_30", "CC_EYEBRW_31", "CC_EYEBRW_32", "CC_EYEBRW_33"}},
    {"wrinkles",
     "FACE_F_SKINA",
     {"Crow's Feet",
      "First Signs",
      "Middle Aged",
      "Worry Lines",
      "Depression",
      "Distinguished",
      "Aged",
      "Weathered",
      "Wrinkled",
      "Sagging",
      "Tough Life",
      "Vintage",
      "Retired",
      "Junkie",
      "Geriatric"}},
    {"makeup", "HAIR_OPTION_2", {"CC_MKUP_0",  "CC_MKUP_1",  "CC_MKUP_2",  "CC_MKUP_3",  "CC_MKUP_4",  "CC_MKUP_5",  "CC_MKUP_6",  "CC_MKUP_7",  "CC_MKUP_8",  "CC_MKUP_9",  "CC_MKUP_10",
                                 "CC_MKUP_11", "CC_MKUP_12", "CC_MKUP_13", "CC_MKUP_14", "CC_MKUP_15", "CC_MKUP_16", "CC_MKUP_17", "CC_MKUP_18", "CC_MKUP_19", "CC_MKUP_20", "CC_MKUP_21",
                                 "CC_MKUP_22", "CC_MKUP_23", "CC_MKUP_24", "CC_MKUP_25", "CC_MKUP_26", "CC_MKUP_27", "CC_MKUP_28", "CC_MKUP_29", "CC_MKUP_30", "CC_MKUP_31", "CC_MKUP_32",
                                 "CC_MKUP_33", "CC_MKUP_34", "CC_MKUP_35", "CC_MKUP_36", "CC_MKUP_37", "CC_MKUP_38", "CC_MKUP_39", "CC_MKUP_40", "CC_MKUP_41"}}, // Makeup & face paint HAIR_OPTION_2
    {"blush", "FACE_F_BLUSH", {"CC_BLUSH_0", "CC_BLUSH_1", "CC_BLUSH_2", "CC_BLUSH_3", "CC_BLUSH_4", "CC_BLUSH_5", "CC_BLUSH_6"}},
    {"complexion", "FACE_F_SKC", {"Rosy Cheeks", "Stubble Rash", "Hot Flush", "Sunburn", "Bruised", "Alcoholic", "Patchy", "Totem", "Blood Vessels", "Damaged", "Pale", "Ghostly"}},
    {"blemishes", "FACE_F_SKINB", {"Measles",    "Pimples", "Spots",   "Break Out", "Blackheads", "Build Up", "Pustules", "Zits",   "Full Acne", "Acne",          "Cheek Rash",
                                   "Face Rash",  "Picker",  "Puberty", "Eyesore",   "Chin Rash",  "Two Face", "T Zone",   "Greasy", "Marked",    "Acne Scarring", "Full Acne Scarring",
                                   "Cold Sores", "Impetigo"}},
    {"lipstick",
     "FACE_F_LIPST",
     {"CC_LIPSTICK_0", "CC_LIPSTICK_1", "CC_LIPSTICK_2", "CC_LIPSTICK_3", "CC_LIPSTICK_4", "CC_LIPSTICK_5", "CC_LIPSTICK_6", "CC_LIPSTICK_7", "CC_LIPSTICK_8", "CC_LIPSTICK_9"}},
    {"spots",
     "FACE_F_MOLE",
     {"Cherub",
      "All Over",
      "Irregular",
      "Dot Dash",
      "Over the Bridge",
      "Baby Doll",
      "Pixie",
      "Sun Kissed",
      "Beauty Marks",
      "Line Up",
      "Modelesque",
      "Occasional",
      "Speckled",
      "Rain Drops",
      "Double Dip",
      "One Sided",
      "Pairs",
      "Growth"}},
    {"chest_hair",
     "Chest Hair",
     {"CC_BODY_1_0",
      "CC_BODY_1_1",
      "CC_BODY_1_2",
      "CC_BODY_1_3",
      "CC_BODY_1_4",
      "CC_BODY_1_5",
      "CC_BODY_1_6",
      "CC_BODY_1_7",
      "CC_BODY_1_8",
      "CC_BODY_1_9",
      "CC_BODY_1_10",
      "CC_BODY_1_11",
      "CC_BODY_1_12",
      "CC_BODY_1_13",
      "CC_BODY_1_14",
      "CC_BODY_1_15",
      "CC_BODY_1_16",
      "CC_BODY_1_17"}},
    {"chest_blemishes", "Chest Blemishes", {}},
    {"chest_blemishes_2", "Chest Blemishes 2", {}},
}};

struct FacialFeatureCaption
{
    std::string_view localizationId;
    std::string_view englishText;
};

static const std::array<FacialFeatureCaption, 20> facialFeatureCaptions = {{
    {"nose_width", "Nose Width"},
    {"nose_bottom_height", "Nose Bottom Height"},
    {"nose_tip_length", "Nose Tip Length"},
    {"nose_bridge_depth", "Nose Bridge Depth"},
    {"nose_tip_height", "Nose Tip Height"},
    {"nose_broken", "Nose Broken"},
    {"brow_height", "Brow Height"},
    {"brow_depth", "Brow Depth"},
    {"cheekbone_height", "Cheekbone Height"},
    {"cheekbone_width", "Cheekbone Width"},
    {"cheek_depth", "Cheek Depth"},
    {"eye_size", "Eye Size"},
    {"lip_thickness", "Lip Thickness"},
    {"jaw_width", "Jaw Width"},
    {"jaw_shape", "Jaw Shape"},
    {"chin_height", "Chin Height"},
    {"chin_depth", "Chin Depth"},
    {"chin_width", "Chin Width"},
    {"chin_indent", "Chin Indent"},
    {"neck_width", "Neck Width"},
}};
#pragma endregion

namespace PedFaceGen
{
// [gender][skin]: [0=Male/1=Female][0=White,1=Black,2=Hispanic,3=Asian,4=Arab,5=Pakistani]
const std::vector<int> parentIdsByGenderSkin[2][6] = {
    {
        // Male
        {0, 1, 12, 13, 42, 43, 44, 45}, // White
        {2, 3, 14, 15},                 // Black
        {4, 5, 16},                     // Hispanic
        {6, 7, 17, 18},                 // Asian
        {10, 11, 20},                   // Arab
        {8, 9, 19}                      // Pakistani
    },
    {
        // Female
        {21, 22, 33, 34}, // White
        {23, 24, 35, 36}, // Black
        {25, 26, 37},     // Hispanic
        {27, 28, 38, 39}, // Asian
        {31, 32, 41},     // Arab
        {29, 30, 40}      // Pakistani
    }
};

FaceGeneratorSettings settings;
} // namespace PedFaceGen

inline int getMaxShapeAndSkinIds()
{
    return g_unlockMaxIDs ? 255 : 46;
}

std::uint8_t GetPedHeadOverlayColourType(const PedHeadOverlay& overlayIndex)
{
    switch (overlayIndex)
    {
    case PedHeadOverlay::Eyebrows:
    case PedHeadOverlay::Beard:
    case PedHeadOverlay::ChestHair:
    case PedHeadOverlay::Makeup:
        return 1;
    case PedHeadOverlay::Blush:
    case PedHeadOverlay::Lipstick:
        return 2;
    default:
        return 0;
    }
}

bool DoesPedModelSupportHeadFeatures(const GTAmodel::Model& pedModel)
{
    return pedModel.hash == PedHash::FreemodeMale01 || pedModel.hash == PedHash::FreemodeFemale01;
}

static void DrawFaceStats(float topY);
static std::string FormatMixValue(float mixValue);
static void DrawOverlayStats(float topY, int focusSlot = -1);
static RGBA GetHairTintColor(int tintId);
static RGBA GetMakeupTintColor(int tintId);

static std::string TranslateLocalizedOverlayText(std::string_view localizationPrefix, std::string_view captionId, const std::string& fallback)
{
    const std::string translatedCaption = Language::TranslateStable(localizationPrefix, captionId, fallback);
    if (translatedCaption != fallback)
        return translatedCaption;

    return Game::GetGXTEntry(Language::TranslateToSelected(fallback), fallback);
}

static std::string TranslateOverlayTitle(const HeadOverlayCaptions& overlay)
{
    const std::string localizationPrefix = "components.head.overlays." + std::string(overlay.localizationId);
    return TranslateLocalizedOverlayText(localizationPrefix, "title", overlay.title);
}

static std::string TranslateOverlayVariation(const HeadOverlayCaptions& overlay, const std::string& variation)
{
    const std::string localizationPrefix = "components.head.overlays." + std::string(overlay.localizationId) + ".variation";
    return TranslateLocalizedOverlayText(localizationPrefix, variation, variation);
}

static std::string TranslateFaceFeature(const FacialFeatureCaption& caption)
{
    return Language::TranslateStable("components.face.feature", caption.localizationId, caption.englishText);
}

void Sub_Main()
{
    GTAped ped = g_activePedHandle;
    Model pedModel = ped.Model();

    WardrobeCamera::SetFraming(ped, WardrobeCamera::Framing::Head);

    if (!ped.Exists() || !DoesPedModelSupportHeadFeatures(pedModel.hash))
    {
        auto pit = pedHeadFeaturesByPed.find(ped.Handle());
        if (pit != pedHeadFeaturesByPed.end())
        {
            pedHeadFeaturesByPed.erase(pit);
        }
        Menu::SetPreviousMenu();
        Game::Print::PrintBottomLeft(Language::Translate("components.notifications.notFreemodePed"));
        return;
    }

    auto [headFeatures, inserted] = pedHeadFeaturesByPed.try_emplace(ped.Handle());
    selectedPedHeadFeatures = &headFeatures->second;
    if (inserted)
    {
        ped.GetHairTint(selectedPedHeadFeatures->hairColour, selectedPedHeadFeatures->hairHighlightColour);
        selectedPedHeadFeatures->eyeColour = ped.GetEyeColour();

        for (int featureIndex = 0; featureIndex < selectedPedHeadFeatures->facialFeatures.size(); ++featureIndex)
            ped.GetFaceFeatureValue(featureIndex, selectedPedHeadFeatures->facialFeatures[featureIndex]);

        for (int overlayIndex = 0; overlayIndex < selectedPedHeadFeatures->overlays.size(); ++overlayIndex)
        {
            PedHeadOverlayData overlayData;
            if (!ped.GetHeadOverlayData(overlayIndex, overlayData))
                continue;

            auto& overlaySelection = selectedPedHeadFeatures->overlays[overlayIndex];
            overlaySelection.opacity = overlayData.opacity;
            overlaySelection.primaryColour = overlayData.primaryColour;
            overlaySelection.secondaryColour = overlayData.secondaryColour;
        }
    }

    int maxIds = getMaxShapeAndSkinIds();

    auto headBlend = ped.GetHeadBlendData();
    if (headBlend.shapeFirstID < 0 || headBlend.shapeFirstID > maxIds || headBlend.shapeSecondID < 0 || headBlend.shapeSecondID > maxIds || headBlend.shapeThirdID < 0 ||
        headBlend.shapeThirdID > maxIds || headBlend.skinFirstID < 0 || headBlend.skinFirstID > maxIds || headBlend.skinSecondID < 0 || headBlend.skinSecondID > maxIds || headBlend.skinThirdID < 0 ||
        headBlend.skinThirdID > maxIds)
    {
        headBlend.shapeFirstID = 0;
        headBlend.shapeSecondID = 0;
        headBlend.shapeThirdID = 0;
        headBlend.skinFirstID = 1;
        headBlend.skinSecondID = 1;
        headBlend.skinThirdID = 1;
        headBlend.shapeMix = 0.0f;
        headBlend.skinMix = 0.0f;
        headBlend.thirdMix = 0.0f;
        headBlend.isParent = false;
        ped.SetHeadBlendData(headBlend);
    }

    int maxHairColours = GET_NUM_PED_HAIR_TINTS() - 1;
    int maxEyeColours = 32;

    bool hairColourPlus = false, hairColourMinus = false;
    bool hairStreaksPlus = false, hairStreaksMinus = false;
    bool eyeColourPlus = false, eyeColourMinus = false;

    AddTitle(Language::Translate("components.head.title"));

    DrawFaceStats(StatsPanel::MenuTopY());

    AddOption(Language::Translate("components.head.overlays"), null, nullFunc, SUB::PED_HEAD_OVERLAYS);
    AddOption(Language::Translate("components.head.facialFeatures"), null, nullFunc, SUB::PED_HEAD_FACE_FEATURES);
    AddOption(Language::Translate("components.head.shapeSkin"), null, nullFunc, SUB::PED_HEAD_SKIN_TONE);
    AddOption(Language::Translate("components.head.faceGenerator"), null, nullFunc, SUB::PED_HEAD_FACE_GENERATOR);

    AddBreak(Language::Translate("components.head.hair"));
    AddNumber(Language::Translate("components.head.hairColour"), selectedPedHeadFeatures->hairColour, 0, null, hairColourPlus, hairColourMinus);
    AddNumber(Language::Translate("components.head.hairStreaksColour"), selectedPedHeadFeatures->hairHighlightColour, 0, null, hairStreaksPlus, hairStreaksMinus);
    AddOptionDescription(Language::Translate("components.head.hairHighlights.description"));

    AddBreak(Language::Translate("components.head.eyes"));
    AddNumber(Game::GetGXTEntry("FACE_APP_EYE", "Eye Colour"), selectedPedHeadFeatures->eyeColour, 0, null, eyeColourPlus, eyeColourMinus);

    if (hairColourPlus || hairColourMinus)
    {
        selectedPedHeadFeatures->hairColour = static_cast<int>(cycleFloat(static_cast<float>(selectedPedHeadFeatures->hairColour), hairColourPlus, 0.0f, static_cast<float>(maxHairColours), 1.0f));
        ped.SetHairTint(selectedPedHeadFeatures->hairColour, selectedPedHeadFeatures->hairHighlightColour);
    }

    if (hairStreaksPlus || hairStreaksMinus)
    {
        selectedPedHeadFeatures->hairHighlightColour =
            static_cast<int>(cycleFloat(static_cast<float>(selectedPedHeadFeatures->hairHighlightColour), hairStreaksPlus, 0.0f, static_cast<float>(maxHairColours), 1.0f));
        ped.SetHairTint(selectedPedHeadFeatures->hairColour, selectedPedHeadFeatures->hairHighlightColour);
    }

    if (eyeColourPlus || eyeColourMinus)
    {
        selectedPedHeadFeatures->eyeColour = static_cast<int>(cycleFloat(static_cast<float>(selectedPedHeadFeatures->eyeColour), eyeColourPlus, 0.0f, static_cast<float>(maxEyeColours), 1.0f));
        ped.SetEyeColour(SYSTEM::ROUND((float)selectedPedHeadFeatures->eyeColour));
    }
}

static void DrawOverlayStats(float topY, int focusSlot)
{
    GTAped ped = g_activePedHandle;

    bool firstShownSection = true;
    std::vector<StatsPanel::Row> rows;
    for (size_t i = 0; i < vCaptions_headOverlays.size(); i++)
    {
        if (focusSlot >= 0 && (int)i != focusSlot)
            continue;
        int overlayIndex = (int)i;
        PedHeadOverlayData overlayData;
        if (!ped.GetHeadOverlayData(overlayIndex, overlayData))
            continue;
        int overlayValue = overlayData.drawableIndex;
        if (overlayValue == 255 && (int)i != focusSlot)
            continue;
        auto& overlays = selectedPedHeadFeatures->overlays[overlayIndex];

        if (!firstShownSection)
            rows.push_back(StatsPanel::SeparatorRow{});
        firstShownSection = false;
        const auto& overlayCaptions = vCaptions_headOverlays[i];
        rows.push_back(StatsPanel::TitleRow{TranslateOverlayTitle(overlayCaptions)});
        if (overlayValue == 255)
        {
            rows.push_back(StatsPanel::StatRow{Language::Translate("components.stats.variation"), Language::Translate("components.head.overlay.none")});
        }
        else
        {
            std::string variationText = Language::Format("components.head.overlay.style", {{"index", overlayValue}});
            auto& variationNames = overlayCaptions.variations;
            if (overlayValue >= 0 && (size_t)overlayValue < variationNames.size() && !variationNames[overlayValue].empty())
                variationText = TranslateOverlayVariation(overlayCaptions, variationNames[overlayValue]) + " (" + std::to_string(overlayValue) + ")";
            rows.push_back(StatsPanel::StatRow{Language::Translate("components.stats.variation"), variationText});
            rows.push_back(StatsPanel::StatRow{Language::Translate("components.stats.opacity"), FormatMixValue(overlays.opacity)});
            int colourType = GetPedHeadOverlayColourType((PedHeadOverlay)overlayIndex);
            if (colourType != 0)
            {
                std::string colourText = overlays.primaryColour >= 0 ? std::to_string(overlays.primaryColour) : "---";
                std::string secondaryText = overlays.secondaryColour >= 0 ? std::to_string(overlays.secondaryColour) : "---";
                if (colourType == 1)
                    rows.push_back(
                        StatsPanel::DoubleColorRow{
                            Language::Translate("components.stats.colour"),
                            GetHairTintColor(overlays.primaryColour),
                            colourText,
                            Language::Translate("components.stats.secondary"),
                            GetHairTintColor(overlays.secondaryColour),
                            secondaryText
                        }
                    );
                else
                    rows.push_back(
                        StatsPanel::DoubleColorRow{
                            Language::Translate("components.stats.colour"),
                            GetMakeupTintColor(overlays.primaryColour),
                            colourText,
                            Language::Translate("components.stats.secondary"),
                            GetMakeupTintColor(overlays.secondaryColour),
                            secondaryText
                        }
                    );
            }
        }
    }

    StatsPanel::Draw(topY, 0.150f, rows);
}

void Sub_HeadOverlays()
{
    auto& overlayIndex = selectedOverlayIndex;
    AddTitle(Language::Translate("components.head.overlays"));

    DrawOverlayStats(StatsPanel::MenuTopY());

    for (UINT i = 0; i < vCaptions_headOverlays.size(); i++)
    {
        bool bOverlayNamePressed = false;
        AddOption(TranslateOverlayTitle(vCaptions_headOverlays[i]), bOverlayNamePressed, nullFunc, -1, true, true);
        if (bOverlayNamePressed)
        {
            overlayIndex = i;
            Menu::pendingSubmenu = SUB::PED_HEAD_OVERLAY_SETTINGS;
        }
    }
}

void ApplyHeadOverlayTint(GTAped ped, int overlayIndex, int colourType, int primary, int secondary)
{
    if (primary < 0 || colourType == 0)
    {
        ped.SetHeadOverlayTint(overlayIndex, 0, 0, 0);
    }
    else
    {
        ped.SetHeadOverlayTint(overlayIndex, colourType, primary, secondary);
    }
}

void Sub_HeadOverlays_InItem()
{
    auto& overlayIndex = selectedOverlayIndex;
    GTAped ped = g_activePedHandle;

    auto colourType = GetPedHeadOverlayColourType((PedHeadOverlay)overlayIndex);
    bool bColoursAvailable = (colourType != 0);

    auto& currentOverlayData = selectedPedHeadFeatures->overlays[overlayIndex];
    PedHeadOverlayData overlayData;
    if (!ped.GetHeadOverlayData(overlayIndex, overlayData))
        return;
    int overlayValue = overlayData.drawableIndex;
    int maxOverlays = ped.GetHeadOverlayDrawableCount(overlayIndex) - 1;
    int maxColours = 64;

    bool overlayPlus = false, overlayMinus = false;
    bool opacityPlus = false, opacityMinus = false;

    const auto& overlayCaptions = vCaptions_headOverlays[overlayIndex];
    AddTitle(TranslateOverlayTitle(overlayCaptions));

    DrawOverlayStats(StatsPanel::MenuTopY(), overlayIndex);

    // VARIATION
    std::vector<std::string> variationNames;
    variationNames.reserve(overlayCaptions.variations.size());
    for (const std::string& variationName : overlayCaptions.variations)
        variationNames.push_back(TranslateOverlayVariation(overlayCaptions, variationName));
    AddTexter(Language::Translate("components.variation"), overlayValue, variationNames, null, overlayPlus, overlayMinus);
    if (overlayPlus)
    {
        if (overlayValue < maxOverlays)
        {
            overlayValue++;
        }
        else
        {
            overlayValue = overlayValue == 255 ? 0 : 255;
        }

        ped.SetHeadOverlay(overlayIndex, overlayValue, currentOverlayData.opacity);
        ApplyHeadOverlayTint(ped, overlayIndex, colourType, currentOverlayData.primaryColour, currentOverlayData.secondaryColour);
    }

    if (overlayMinus)
    {
        if (overlayValue > 0)
        {
            overlayValue = overlayValue > maxOverlays ? maxOverlays : overlayValue - 1;
        }
        else
        {
            overlayValue = 255;
        }

        ped.SetHeadOverlay(overlayIndex, overlayValue, currentOverlayData.opacity);
        ApplyHeadOverlayTint(ped, overlayIndex, colourType, currentOverlayData.primaryColour, currentOverlayData.secondaryColour);
    }

    // OPACITY
    AddNumber(Game::GetGXTEntry("FACE_OPAC", "Opacity"), currentOverlayData.opacity, 3, null, opacityPlus, opacityMinus);
    if (opacityPlus)
    {
        if (currentOverlayData.opacity < 1.0f)
        {
            currentOverlayData.opacity += 0.01f;
            ped.SetHeadOverlay(overlayIndex, overlayValue, currentOverlayData.opacity);
        }
    }

    if (opacityMinus)
    {
        if (currentOverlayData.opacity > 0.0f)
        {
            currentOverlayData.opacity -= 0.01f;
            ped.SetHeadOverlay(overlayIndex, overlayValue, currentOverlayData.opacity);
        }
    }

    if (bColoursAvailable)
    {
        bool colourPlus = false, colourMinus = false;
        bool secondaryColourPlus = false, secondaryColourMinus = false;

        // PRIMARY COLOUR
        AddNumber(Game::GetGXTEntry("CMOD_COL0_0", "Primary Colour"), currentOverlayData.primaryColour, 0, null, colourPlus, colourMinus);
        if (colourPlus)
        {
            if (currentOverlayData.primaryColour < maxColours)
            {
                currentOverlayData.primaryColour++;
            }
            else
            {
                currentOverlayData.primaryColour = -1;
            }

            ApplyHeadOverlayTint(ped, overlayIndex, colourType, currentOverlayData.primaryColour, currentOverlayData.secondaryColour);
        }

        if (colourMinus)
        {
            if (currentOverlayData.primaryColour > -1)
            {
                currentOverlayData.primaryColour--;
            }
            else
            {
                currentOverlayData.primaryColour = maxColours;
            }

            ApplyHeadOverlayTint(ped, overlayIndex, colourType, currentOverlayData.primaryColour, currentOverlayData.secondaryColour);
        }

        // SECONDARY COLOUR
        if (currentOverlayData.primaryColour > -1)
        {
            AddNumber(Game::GetGXTEntry("CMOD_COL0_1", "Secondary Colour"), currentOverlayData.secondaryColour, 0, null, secondaryColourPlus, secondaryColourMinus);
        }

        if (secondaryColourPlus)
        {
            if (currentOverlayData.secondaryColour < maxColours)
            {
                currentOverlayData.secondaryColour++;
            }

            ApplyHeadOverlayTint(ped, overlayIndex, colourType, currentOverlayData.primaryColour, currentOverlayData.secondaryColour);
        }

        if (secondaryColourMinus)
        {
            if (currentOverlayData.secondaryColour > -1)
            {
                currentOverlayData.secondaryColour--;
            }

            ApplyHeadOverlayTint(ped, overlayIndex, colourType, currentOverlayData.primaryColour, currentOverlayData.secondaryColour);
        }
    }
}

// Hair tint palette (tint index -> RGB), source: https://github.com/DurtyFree/gta-v-data-dumps/blob/master/pedHairColors.json
static const RGBA hairTintPalette[] = {RGBA(28, 31, 33, 255),    RGBA(39, 42, 44, 255),   RGBA(49, 46, 44, 255),    RGBA(53, 38, 28, 255),    RGBA(75, 50, 31, 255),    RGBA(92, 59, 36, 255),
                                       RGBA(109, 76, 53, 255),   RGBA(107, 80, 59, 255),  RGBA(118, 92, 69, 255),   RGBA(127, 104, 78, 255),  RGBA(153, 129, 93, 255),  RGBA(167, 147, 105, 255),
                                       RGBA(175, 156, 112, 255), RGBA(187, 160, 99, 255), RGBA(214, 185, 123, 255), RGBA(218, 195, 142, 255), RGBA(159, 127, 89, 255),  RGBA(132, 80, 57, 255),
                                       RGBA(104, 43, 31, 255),   RGBA(97, 18, 12, 255),   RGBA(100, 15, 10, 255),   RGBA(124, 20, 15, 255),   RGBA(160, 46, 25, 255),   RGBA(182, 75, 40, 255),
                                       RGBA(162, 80, 47, 255),   RGBA(170, 78, 43, 255),  RGBA(98, 98, 98, 255),    RGBA(128, 128, 128, 255), RGBA(170, 170, 170, 255), RGBA(197, 197, 197, 255),
                                       RGBA(70, 57, 85, 255),    RGBA(90, 63, 107, 255),  RGBA(118, 60, 118, 255),  RGBA(237, 116, 227, 255), RGBA(235, 75, 147, 255),  RGBA(242, 153, 188, 255),
                                       RGBA(4, 149, 158, 255),   RGBA(2, 95, 134, 255),   RGBA(2, 57, 116, 255),    RGBA(63, 161, 106, 255),  RGBA(33, 124, 97, 255),   RGBA(24, 92, 85, 255),
                                       RGBA(182, 192, 52, 255),  RGBA(112, 169, 11, 255), RGBA(67, 157, 19, 255),   RGBA(220, 184, 87, 255),  RGBA(229, 177, 3, 255),   RGBA(230, 145, 2, 255),
                                       RGBA(242, 136, 49, 255),  RGBA(251, 128, 87, 255), RGBA(226, 139, 88, 255),  RGBA(209, 89, 60, 255),   RGBA(206, 49, 32, 255),   RGBA(173, 9, 3, 255),
                                       RGBA(136, 3, 2, 255),     RGBA(31, 24, 20, 255),   RGBA(41, 31, 25, 255),    RGBA(46, 34, 27, 255),    RGBA(55, 41, 30, 255),    RGBA(46, 34, 24, 255),
                                       RGBA(35, 27, 21, 255),    RGBA(2, 2, 2, 255),      RGBA(112, 108, 102, 255), RGBA(157, 122, 80, 255)};
static const int hairTintPaletteCount = sizeof(hairTintPalette) / sizeof(hairTintPalette[0]);

static RGBA GetHairTintColor(int tintId)
{
    if (tintId >= 0 && tintId < hairTintPaletteCount)
        return hairTintPalette[tintId];
    return RGBA(128, 128, 128, 255);
}

// Makeup tint palette (tint index -> RGB), source: https://wiki.rage.mp/wiki/Makeup_Colors
static const RGBA makeupTintPalette[] = {RGBA(153, 37, 50, 255),   RGBA(200, 57, 93, 255),  RGBA(189, 81, 108, 255),  RGBA(184, 99, 122, 255),  RGBA(166, 82, 107, 255),  RGBA(177, 67, 76, 255),
                                         RGBA(127, 49, 51, 255),   RGBA(164, 100, 93, 255), RGBA(193, 135, 121, 255), RGBA(203, 160, 150, 255), RGBA(198, 145, 143, 255), RGBA(171, 111, 99, 255),
                                         RGBA(176, 96, 80, 255),   RGBA(168, 76, 51, 255),  RGBA(180, 113, 120, 255), RGBA(202, 127, 146, 255), RGBA(237, 156, 190, 255), RGBA(231, 117, 164, 255),
                                         RGBA(222, 62, 129, 255),  RGBA(179, 76, 110, 255), RGBA(113, 39, 57, 255),   RGBA(79, 31, 42, 255),    RGBA(170, 34, 47, 255),   RGBA(222, 32, 52, 255),
                                         RGBA(207, 8, 19, 255),    RGBA(229, 84, 112, 255), RGBA(220, 63, 181, 255),  RGBA(194, 39, 178, 255),  RGBA(160, 28, 169, 255),  RGBA(110, 24, 117, 255),
                                         RGBA(115, 20, 101, 255),  RGBA(86, 22, 92, 255),   RGBA(109, 26, 157, 255),  RGBA(27, 55, 113, 255),   RGBA(29, 78, 167, 255),   RGBA(30, 116, 187, 255),
                                         RGBA(33, 163, 206, 255),  RGBA(37, 194, 210, 255), RGBA(35, 204, 165, 255),  RGBA(39, 192, 125, 255),  RGBA(27, 156, 50, 255),   RGBA(20, 134, 4, 255),
                                         RGBA(112, 208, 65, 255),  RGBA(197, 234, 52, 255), RGBA(225, 227, 47, 255),  RGBA(255, 221, 38, 255),  RGBA(250, 192, 38, 255),  RGBA(247, 138, 39, 255),
                                         RGBA(254, 89, 16, 255),   RGBA(190, 110, 25, 255), RGBA(247, 201, 127, 255), RGBA(251, 229, 192, 255), RGBA(245, 245, 245, 255), RGBA(179, 180, 179, 255),
                                         RGBA(145, 145, 145, 255), RGBA(86, 78, 78, 255),   RGBA(24, 14, 14, 255),    RGBA(88, 150, 158, 255),  RGBA(77, 111, 140, 255),  RGBA(26, 43, 85, 255),
                                         RGBA(160, 126, 107, 255), RGBA(130, 99, 85, 255),  RGBA(109, 83, 70, 255),   RGBA(62, 45, 39, 255)};
static const int makeupTintPaletteCount = sizeof(makeupTintPalette) / sizeof(makeupTintPalette[0]);

static RGBA GetMakeupTintColor(int tintId)
{
    if (tintId >= 0 && tintId < makeupTintPaletteCount)
        return makeupTintPalette[tintId];
    return RGBA(128, 128, 128, 255);
}

static std::string FormatMixValue(float mixValue)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.2f", mixValue);
    return buffer;
}

static void DrawFaceStats(float topY)
{
    GTAped ped = g_activePedHandle;

    PedHeadBlendData blendData;
    if (!ped.GetHeadBlendData(blendData))
        return;

    std::vector<StatsPanel::Row> rows;
    rows.push_back(
        StatsPanel::DoubleStatRow{
            Language::Translate("components.stats.shapeFather"), std::to_string(blendData.shapeFirstID), Language::Translate("components.stats.toneFather"), std::to_string(blendData.skinFirstID)
        }
    );
    rows.push_back(
        StatsPanel::DoubleStatRow{
            Language::Translate("components.stats.shapeMother"), std::to_string(blendData.shapeSecondID), Language::Translate("components.stats.toneMother"), std::to_string(blendData.skinSecondID)
        }
    );
    rows.push_back(
        StatsPanel::DoubleStatRow{
            Language::Translate("components.stats.shapeMix"), FormatMixValue(blendData.shapeMix), Language::Translate("components.stats.toneMix"), FormatMixValue(blendData.skinMix)
        }
    );
    rows.push_back(
        StatsPanel::DoubleStatRow{
            Language::Translate("components.stats.shapeAncestor"), std::to_string(blendData.shapeThirdID), Language::Translate("components.stats.toneAncestor"), std::to_string(blendData.skinThirdID)
        }
    );
    rows.push_back(StatsPanel::DoubleStatRow{Language::Translate("components.stats.ancestorMix"), FormatMixValue(blendData.thirdMix), "", ""});
    rows.push_back(StatsPanel::SeparatorRow{});
    for (size_t i = 0; i < facialFeatureCaptions.size(); i += 2)
    {
        std::string rightLabel = "";
        std::string rightValue = "";
        if (i + 1 < facialFeatureCaptions.size())
        {
            rightLabel = TranslateFaceFeature(facialFeatureCaptions[i + 1]);
            rightValue = FormatMixValue(selectedPedHeadFeatures->facialFeatures[i + 1]);
        }
        std::string leftLabel = TranslateFaceFeature(facialFeatureCaptions[i]);
        rows.push_back(StatsPanel::DoubleStatRow{leftLabel, FormatMixValue(selectedPedHeadFeatures->facialFeatures[i]), rightLabel, rightValue});
    }
    rows.push_back(StatsPanel::SeparatorRow{});
    rows.push_back(
        StatsPanel::DoubleColorRow{
            Language::Translate("components.stats.hair"),
            GetHairTintColor(selectedPedHeadFeatures->hairColour),
            std::to_string(selectedPedHeadFeatures->hairColour),
            Language::Translate("components.stats.hairStreaks"),
            GetHairTintColor(selectedPedHeadFeatures->hairHighlightColour),
            std::to_string(selectedPedHeadFeatures->hairHighlightColour)
        }
    );
    rows.push_back(StatsPanel::DoubleStatRow{Language::Translate("components.stats.eyes"), std::to_string(selectedPedHeadFeatures->eyeColour), "", ""});

    StatsPanel::Draw(topY, 0.200f, rows);
}

void Sub_FaceFeatures()
{
    GTAped ped = g_activePedHandle;

    AddTitle(Language::Translate("components.head.facialFeatures"));

    DrawFaceStats(StatsPanel::MenuTopY());

    for (int i = 0; i < facialFeatureCaptions.size(); i++)
    {
        auto& featureValue = selectedPedHeadFeatures->facialFeatures[i];

        bool featurePlus = false, featureMinus = false;
        AddNumber(TranslateFaceFeature(facialFeatureCaptions[i]), featureValue, 2, null, featurePlus, featureMinus);
        if (featurePlus)
        {
            if (featureValue < 1.0f)
            {
                featureValue += 0.05f;
            }
            ped.SetFaceFeatureValue(i, featureValue);
        }

        if (featureMinus)
        {
            if (featureValue > -1.0f)
            {
                featureValue -= 0.05f;
            }
            ped.SetFaceFeatureValue(i, featureValue);
        }
    }
}

void Sub_SkinTone() // HEAD_BLEND
{
    GTAped ped = g_activePedHandle;
    //auto& blendData = _selectedPedHeadFeatures->blendData;
    PedHeadBlendData blendData;
    if (!ped.GetHeadBlendData(blendData))
        return;
    std::vector<std::string> idNames;
    float maxMix = 1.0f;
    float minMix = 0.0f;
    float mixStep = 0.01f;

    AddTitle(Language::Translate("components.head.shapeSkin"));

    DrawFaceStats(StatsPanel::MenuTopY());
    AddToggle(Language::Translate("components.face.unlockIds"), g_unlockMaxIDs);
    AddOptionDescription(Language::Translate("components.face.unlockIds.description"));

    int maxIds = getMaxShapeAndSkinIds();

    // add a texter for a blend ID, handle cycling, and update if changed
    auto addBlendIdTexter = [&](const std::string& label, int& idValue)
    {
        bool plus = false, minus = false;
        AddTexter(label, idValue, idNames, null, plus, minus);
        if (plus || minus)
        {
            idValue = cycleInt(idValue, plus, 0, maxIds);
            ped.SetHeadBlendData(blendData);
        }
    };

    // Shape IDs
    addBlendIdTexter(Language::Translate("components.face.shapeFather"), blendData.shapeFirstID);
    addBlendIdTexter(Language::Translate("components.face.shapeMother"), blendData.shapeSecondID);
    addBlendIdTexter(Language::Translate("components.face.shapeAncestor"), blendData.shapeThirdID);

    // Skin IDs
    addBlendIdTexter(Language::Translate("components.face.toneFather"), blendData.skinFirstID);
    addBlendIdTexter(Language::Translate("components.face.toneMother"), blendData.skinSecondID);
    addBlendIdTexter(Language::Translate("components.face.toneAncestor"), blendData.skinThirdID);

    // Mixes
    AddBreak(Language::Translate("components.face.adjustmentBreak"));

    auto addMixSlider = [&](const std::string& label, float& mixValue)
    {
        bool plus = false, minus = false;
        AddNumber(label, mixValue, 2, null, plus, minus);
        if (plus || minus)
        {
            mixValue = cycleFloat(mixValue, plus, minMix, maxMix, mixStep);
            ped.SetHeadBlendData(blendData);
        }
    };

    addMixSlider(Language::Translate("components.face.shape"), blendData.shapeMix);
    AddOptionDescription(Language::Translate("components.face.shape.description"));
    addMixSlider(Language::Translate("components.face.tone"), blendData.skinMix);
    AddOptionDescription(Language::Translate("components.face.skin.description"));
    addMixSlider(Language::Translate("components.face.ancestor"), blendData.thirdMix);
    AddOptionDescription(Language::Translate("components.face.thirdParent.description"));
}

namespace
{
constexpr int firstModdedParentId = 46;

std::vector<int> BuildParentIdCandidates(int genderFilterIndex, int toneFilterIndex)
{
    std::vector<int> candidateIds;
    int toneIndex = toneFilterIndex - 1;
    int firstGenderIndex = genderFilterIndex == 0 ? 0 : genderFilterIndex - 1;
    int lastGenderIndex = genderFilterIndex == 0 ? 1 : genderFilterIndex - 1;

    for (int genderIndex = firstGenderIndex; genderIndex <= lastGenderIndex; genderIndex++)
    {
        if (toneIndex < 0)
        {
            for (int skinToneIndex = 0; skinToneIndex < 6; skinToneIndex++)
            {
                const auto& parentIds = PedFaceGen::parentIdsByGenderSkin[genderIndex][skinToneIndex];
                candidateIds.insert(candidateIds.end(), parentIds.begin(), parentIds.end());
            }
        }
        else
        {
            const auto& parentIds = PedFaceGen::parentIdsByGenderSkin[genderIndex][toneIndex];
            candidateIds.insert(candidateIds.end(), parentIds.begin(), parentIds.end());
        }
    }

    return candidateIds;
}

void AddModdedParentIds(std::vector<int>& candidateIds, int highestModdedParentId)
{
    for (int parentId = firstModdedParentId; parentId <= highestModdedParentId; parentId++)
        candidateIds.push_back(parentId);
}

int PickRandomParentId(const std::vector<int>& candidateIds)
{
    if (candidateIds.empty())
        return 0;

    return candidateIds[GET_RANDOM_INT_IN_RANGE(0, static_cast<int>(candidateIds.size()))];
}

struct FaceFeatureGroup
{
    const char* localizationKey;
    int firstFeatureIndex;
    int featureCount;
};

constexpr std::array<FaceFeatureGroup, 7> faceFeatureGroups = {{
    {"nose", 0, 6},
    {"brows", 6, 2},
    {"cheeksEyes", 8, 4},
    {"lips", 12, 1},
    {"jaw", 13, 2},
    {"chin", 15, 4},
    {"neck", 19, 1},
}};

template <typename ValueFactory> void ApplyFaceFeatureRange(GTAped& ped, PedHeadFeatureSelection& selection, int firstFeatureIndex, int featureCount, ValueFactory&& getValue)
{
    for (int featureOffset = 0; featureOffset < featureCount; featureOffset++)
    {
        const int featureIndex = firstFeatureIndex + featureOffset;
        const float featureValue = getValue();
        selection.facialFeatures[featureIndex] = featureValue;
        ped.SetFaceFeatureValue(featureIndex, featureValue);
    }
}

std::string GetFeatureGroupName(const FaceFeatureGroup& group)
{
    return Language::Translate(std::string("components.face.group.") + group.localizationKey);
}
} // namespace

void Sub_FaceGenerator()
{
    GTAped ped = g_activePedHandle;

    std::vector<std::string> parentGenderOptions = {
        Language::Translate("components.face.filter.genderAny"), Language::Translate("components.face.filter.male"), Language::Translate("components.face.filter.female")
    };
    std::vector<std::string> parentToneOptions = {
        Language::Translate("components.face.filter.skinAny"),
        Language::Translate("components.face.filter.white"),
        Language::Translate("components.face.filter.black"),
        Language::Translate("components.face.filter.hispanic"),
        Language::Translate("components.face.filter.asian"),
        Language::Translate("components.face.filter.arab"),
        Language::Translate("components.face.filter.pakistani")
    };

    bool randomizeFaceBlendPressed = false;
    bool randomizeShapesPressed = false;
    bool randomizeTonesPressed = false;
    bool randomizeFeaturesPressed = false;
    bool resetAllFeaturesPressed = false;
    bool randomizeFaceAndFeaturesPressed = false;

    // set parent gender filter based on current ped model
    if (PedFaceGen::settings.lastPedModelHash != ped.Model().hash)
    {
        PedFaceGen::settings.lastPedModelHash = ped.Model().hash;
        if (ped.Model().hash == PedHash::FreemodeMale01)
            PedFaceGen::settings.parentGenderFilterIndex = 1;
        else if (ped.Model().hash == PedHash::FreemodeFemale01)
            PedFaceGen::settings.parentGenderFilterIndex = 2;
        else
            PedFaceGen::settings.parentGenderFilterIndex = 0;
    }

    const int allFeatureCount = static_cast<int>(selectedPedHeadFeatures->facialFeatures.size());
    auto randomizeMixValue = []() -> float { return GET_RANDOM_FLOAT_IN_RANGE(0.0f, 1.0f); };

    // --- UI ---
    AddTitle(Language::Translate("components.face.generator"));

    // --- Parents ---
    AddTickol(
        Language::Translate("components.face.useThirdParent"),
        PedFaceGen::settings.includeThirdParent,
        PedFaceGen::settings.includeThirdParent,
        PedFaceGen::settings.includeThirdParent,
        TICKOL::BOXTICK,
        TICKOL::BOXBLANK
    );
    AddOptionDescription(Language::Translate("components.face.randomAncestor.description"));
    PedFaceGen::settings.parentGenderFilterIndex = AddTexterCycler(Language::Translate("components.face.parentFilter"), PedFaceGen::settings.parentGenderFilterIndex, parentGenderOptions);
    AddOptionDescription(Language::Translate("components.face.parentFilter.description"));
    PedFaceGen::settings.parentToneFilterIndex = AddTexterCycler(Language::Translate("components.face.skinColour"), PedFaceGen::settings.parentToneFilterIndex, parentToneOptions);
    AddOptionDescription(Language::Translate("components.face.skinColour.description"));

    // Show non-rockstar parents only if parent / skin color filter is set to "Any" (we don't know the genders/races of modded-parents)
    bool showModdedParents = PedFaceGen::settings.parentGenderFilterIndex == 0 && PedFaceGen::settings.parentToneFilterIndex == 0;
    if (showModdedParents)
    {
        AddNumberStepper(Language::Translate("components.face.maxParent"), PedFaceGen::settings.highestModdedParentId, 0, 1.0, firstModdedParentId, 255.0);
        AddOptionDescription(Language::Translate("components.face.maxParent.description"));
    }

    // --- Randomize ---
    AddBreak(Language::Translate("components.face.randomizeBreak"));
    AddOption(Language::Translate("components.face.randomize"), randomizeFaceBlendPressed);
    AddOptionDescription(Language::Translate("components.face.randomize.description"));
    AddOption(Language::Translate("components.face.randomizeShapes"), randomizeShapesPressed);
    AddOptionDescription(Language::Translate("components.face.randomizeShapes.description"));
    AddOption(Language::Translate("components.face.randomizeTextures"), randomizeTonesPressed);
    AddOptionDescription(Language::Translate("components.face.randomizeTextures.description"));
    AddOption(Language::Translate("components.face.randomizeEverything"), randomizeFaceAndFeaturesPressed);
    AddOptionDescription(Language::Translate("components.face.randomizeEverything.description"));

    if (randomizeFaceBlendPressed || randomizeShapesPressed || randomizeTonesPressed || randomizeFaceAndFeaturesPressed)
    {
        auto candidateIds = BuildParentIdCandidates(PedFaceGen::settings.parentGenderFilterIndex, PedFaceGen::settings.parentToneFilterIndex);
        if (showModdedParents)
            AddModdedParentIds(candidateIds, PedFaceGen::settings.highestModdedParentId);
        if (candidateIds.empty())
            candidateIds.push_back(0);

        PedHeadBlendData headBlendData;
        if (!ped.GetHeadBlendData(headBlendData))
            return;

        if (randomizeFaceBlendPressed || randomizeFaceAndFeaturesPressed)
        {
            headBlendData.shapeFirstID = PickRandomParentId(candidateIds);
            headBlendData.shapeSecondID = PickRandomParentId(candidateIds);
            headBlendData.skinFirstID = PickRandomParentId(candidateIds);
            headBlendData.skinSecondID = PickRandomParentId(candidateIds);
            headBlendData.shapeMix = randomizeMixValue();
            headBlendData.skinMix = randomizeMixValue();
            if (PedFaceGen::settings.includeThirdParent)
            {
                headBlendData.shapeThirdID = PickRandomParentId(candidateIds);
                headBlendData.skinThirdID = PickRandomParentId(candidateIds);
                headBlendData.thirdMix = randomizeMixValue();
            }
        }
        else if (randomizeShapesPressed)
        {
            headBlendData.shapeFirstID = PickRandomParentId(candidateIds);
            headBlendData.shapeSecondID = PickRandomParentId(candidateIds);
            headBlendData.shapeMix = randomizeMixValue();
            if (PedFaceGen::settings.includeThirdParent)
            {
                headBlendData.shapeThirdID = PickRandomParentId(candidateIds);
                headBlendData.thirdMix = randomizeMixValue();
            }
        }
        else if (randomizeTonesPressed)
        {
            headBlendData.skinFirstID = PickRandomParentId(candidateIds);
            headBlendData.skinSecondID = PickRandomParentId(candidateIds);
            headBlendData.skinMix = randomizeMixValue();
            if (PedFaceGen::settings.includeThirdParent)
            {
                headBlendData.skinThirdID = PickRandomParentId(candidateIds);
            }
        }
        ped.SetHeadBlendData(headBlendData);

        if (randomizeFaceAndFeaturesPressed)
            ApplyFaceFeatureRange(ped, *selectedPedHeadFeatures, 0, allFeatureCount, [] { return GET_RANDOM_FLOAT_IN_RANGE(-1.0f, 1.0f); });
        return;
    }

    // --- Facial Features ---
    AddBreak(Language::Translate("components.face.features"));
    AddOption(Language::Translate("components.face.randomizeFeatures"), randomizeFeaturesPressed);
    if (randomizeFeaturesPressed)
    {
        ApplyFaceFeatureRange(ped, *selectedPedHeadFeatures, 0, allFeatureCount, [] { return GET_RANDOM_FLOAT_IN_RANGE(-1.0f, 1.0f); });
        return;
    }

    for (const auto& group : faceFeatureGroups)
    {
        bool randomizeGroupPressed = false;
        AddOption(Language::Format("components.face.randomizeGroup", {{"group", GetFeatureGroupName(group)}}), randomizeGroupPressed);
        if (randomizeGroupPressed)
        {
            ApplyFaceFeatureRange(ped, *selectedPedHeadFeatures, group.firstFeatureIndex, group.featureCount, [] { return GET_RANDOM_FLOAT_IN_RANGE(-1.0f, 1.0f); });
            return;
        }
    }
    AddBreak(Language::Translate("components.face.reset"));
    AddOption(Language::Translate("components.face.resetFeatures"), resetAllFeaturesPressed);
    if (resetAllFeaturesPressed)
    {
        ApplyFaceFeatureRange(ped, *selectedPedHeadFeatures, 0, allFeatureCount, [] { return 0.0f; });
        return;
    }
    for (const auto& group : faceFeatureGroups)
    {
        bool resetGroupPressed = false;
        AddOption(Language::Format("components.face.resetGroup", {{"group", GetFeatureGroupName(group)}}), resetGroupPressed);
        if (resetGroupPressed)
        {
            ApplyFaceFeatureRange(ped, *selectedPedHeadFeatures, group.firstFeatureIndex, group.featureCount, [] { return 0.0f; });
            return;
        }
    }
}
} // namespace PedHeadFeatures_catind

} // namespace sub

#include "..\..\Menu\submenu_switch.h"
#include "..\..\Menu\submenu_enum.h"

REGISTER_SUBMENU(PED_HEAD, sub::PedHeadFeatures_catind::Sub_Main)
REGISTER_SUBMENU(PED_HEAD_OVERLAYS, sub::PedHeadFeatures_catind::Sub_HeadOverlays)
REGISTER_SUBMENU(PED_HEAD_OVERLAY_SETTINGS, sub::PedHeadFeatures_catind::Sub_HeadOverlays_InItem)
REGISTER_SUBMENU(PED_HEAD_FACE_FEATURES, sub::PedHeadFeatures_catind::Sub_FaceFeatures)
REGISTER_SUBMENU(PED_HEAD_SKIN_TONE, sub::PedHeadFeatures_catind::Sub_SkinTone)
REGISTER_SUBMENU(PED_HEAD_FACE_GENERATOR, sub::PedHeadFeatures_catind::Sub_FaceGenerator)
