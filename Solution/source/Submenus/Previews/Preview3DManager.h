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

#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Menu\StatsPanel.h"
#include "PedPreview3D.h"

#include <string>

namespace Previews
{
extern bool g_enabled;

bool DrawModelPreview(
    const std::string& previewKey, EntityType entityType, const GTAmodel::Model& model, float centerX, float centerY, float width, float height, const std::string& previewDataPath = ""
);
bool DrawPedAnimationPreview(const GTAmodel::Model& model, int sourcePedHandle, const PedPreview3D::Animation& animation, float centerX, float centerY, float width, float height);
bool DrawSpoonerScenePreview(const std::string& filePath, float centerX, float centerY, float width, float height);
StatsPanel::Preview3DRow CreateModelPreviewRow(const std::string& previewKey, EntityType entityType, const GTAmodel::Model& model, const std::string& previewDataPath = "");
StatsPanel::Preview3DRow CreatePedAnimationPreviewRow(const GTAmodel::Model& model, int sourcePedHandle, const PedPreview3D::Animation& animation);
void Tick();
void Clear();
void AbandonForProcessExit();
} // namespace Previews
