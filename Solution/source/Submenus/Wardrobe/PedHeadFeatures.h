/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#pragma once

#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Natives\types.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace sub
{
namespace PedHeadFeatures_catind
{
struct PedHeadOverlaySelection
{
    float opacity = 1.0f;
    int primaryColour = -1;
    int secondaryColour = -1;
};

struct PedHeadFeatureSelection
{
    std::array<PedHeadOverlaySelection, 13> overlays;
    std::array<float, 20> facialFeatures{};
    int hairColour = 0;
    int hairHighlightColour = 0;
    int eyeColour = 1;
};

extern std::map<Ped, PedHeadFeatureSelection> pedHeadFeaturesByPed;
extern PedHeadFeatureSelection* selectedPedHeadFeatures;

std::uint8_t GetPedHeadOverlayColourType(const PedHeadOverlay& overlayIndex);
bool DoesPedModelSupportHeadFeatures(const GTAmodel::Model& pedModel);

void Sub_Main();
void Sub_HeadOverlays();
void ApplyHeadOverlayTint(GTAped ped, int overlayIndex, int colourType, int primary, int secondary);
void Sub_HeadOverlays_InItem();
void Sub_FaceFeatures();
void Sub_SkinTone();

namespace PedFaceGen
{
struct FaceGeneratorSettings
{
    bool includeThirdParent = false;
    int parentGenderFilterIndex = 0;
    int parentToneFilterIndex = 0;
    int highestModdedParentId = 46;
    Hash lastPedModelHash = 0;
};

extern const std::vector<int> parentIdsByGenderSkin[2][6];
extern FaceGeneratorSettings settings;
} // namespace PedFaceGen

void Sub_FaceGenerator();
} // namespace PedHeadFeatures_catind
} // namespace sub
