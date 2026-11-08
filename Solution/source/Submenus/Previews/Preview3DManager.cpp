/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "Preview3DManager.h"

#include "..\..\Memory\UI3DScene.h"
#include "..\..\Natives\natives2.h"
#include "PedPreview3D.h"
#include "ModelPreview3D.h"
#include "PropPreview3D.h"
#include "SpoonerScenePreview3D.h"
#include "VehiclePreview3D.h"

#include "..\..\Util\FileLogger.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace Previews
{
namespace
{
constexpr uint32_t kPreviewPreset = 0xAD197067;
constexpr int kPreviewSlot = 0;
constexpr int kPreviewElement = 0;
constexpr uint32_t kPreviewBackground = 0xC8000000;
constexpr float kPreviewFitMargin = 1.08f;
constexpr float kPedPreviewCameraDistance = 6.5f;
constexpr float kVehiclePreviewCameraFov = 55.0f;
constexpr float kVehiclePreviewDistanceMultiplier = 1.75f;
constexpr DWORD kPreviewInactiveTimeoutMilliseconds = 5000;

EntityType g_activeEntityType = EntityType::ALL;
Hash g_activeModelHash = 0;
std::string g_activePreviewKey;
std::unique_ptr<Preview3DContent> g_activePreview;
DWORD g_lastDrawTimeMilliseconds = 0;
float g_rotationDegrees = 0.0f;
bool g_previewFailureLogged = false;

void LogPreviewFailureOnce(const std::string& stage)
{
    if (g_previewFailureLogged)
        return;
    addlog(ige::LogType::LOG_WARNING, "Preview3D: " + stage + " for model hash " + std::to_string(g_activeModelHash));
    g_previewFailureLogged = true;
}

std::unique_ptr<Preview3DContent> CreatePreview(EntityType entityType, const GTAmodel::Model& model, const std::string& previewDataPath)
{
    switch (entityType)
    {
    case EntityType::PED:
        if (!model.IsPed())
            return nullptr;
        return std::make_unique<PedPreview3D>(model, previewDataPath);
    case EntityType::VEHICLE:
        return std::make_unique<VehiclePreview3D>(model);
    case EntityType::PROP:
        if (model.IsPed() || model.IsVehicle())
            return nullptr;
        return std::make_unique<PropPreview3D>(model);
    default:
        return nullptr;
    }
}

bool SetPreviewViewport(const Preview3DContent& preview, float centerX, float centerY, float width, float height, UI3DScene::ElementPatch& patch)
{
    if (!UI3DScene::ReadPresetElement(kPreviewPreset, kPreviewElement, patch))
        return false;

    const float aspectRatio = GET_ASPECT_RATIO(false);
    const float aspectScale = aspectRatio > 0.0f ? (16.0f / 9.0f) / aspectRatio : 1.0f;
    const float previewAspect = height > 0.0f ? width * aspectRatio / height : aspectRatio;
    const float originalCameraFov = UI3DScene::GetOriginalCameraFov();
    const float cameraFov = preview.GetRootType() == EntityType::VEHICLE ? kVehiclePreviewCameraFov : originalCameraFov;
    const float tangentVertical = (std::max)(std::tan(cameraFov * 0.00872664626f), 0.01f);
    const float originalLength = std::sqrt(patch.pos[0] * patch.pos[0] + patch.pos[1] * patch.pos[1] + patch.pos[2] * patch.pos[2]);
    Vector3 minimum;
    Vector3 maximum;
    float directionX = 0.0f;
    float directionY = 1.0f;
    float directionZ = 0.0f;
    if (originalLength > 0.01f)
    {
        directionX = patch.pos[0] / originalLength;
        directionY = patch.pos[1] / originalLength;
        directionZ = patch.pos[2] / originalLength;
    }

    float rightX = directionY;
    float rightY = -directionX;
    const float rightLength = std::sqrt(rightX * rightX + rightY * rightY);
    if (rightLength > 0.001f)
    {
        rightX /= rightLength;
        rightY /= rightLength;
    }
    else
    {
        rightX = 1.0f;
        rightY = 0.0f;
    }
    const float upX = rightY * directionZ;
    const float upY = -rightX * directionZ;
    const float upZ = rightX * directionY - rightY * directionX;

    float distance = originalLength;
    if (preview.IsCompositeScene())
    {
        const float sceneRadius = (std::max)(preview.GetSceneRadius(), 0.5f);
        minimum = Vector3(-sceneRadius, -sceneRadius, -sceneRadius);
        maximum = Vector3(sceneRadius, sceneRadius, sceneRadius);
        const float halfVerticalFov = std::atan(tangentVertical);
        const float halfHorizontalFov = std::atan(tangentVertical * (std::max)(previewAspect, 0.1f));
        const float limitingHalfFov = (std::min)(halfVerticalFov, halfHorizontalFov);
        distance = (std::max)(distance, sceneRadius / (std::max)(std::sin(limitingHalfFov), 0.01f));
    }
    else
    {
        GetModelPreviewBounds(preview.GetFramingModel(), minimum, maximum);
        const Vector3 boundsCenter = (minimum + maximum) * 0.5f;
        const float headingRadians = g_rotationDegrees * 0.017453292f;
        const float cosine = std::cos(headingRadians);
        const float sine = std::sin(headingRadians);
        const float tangentHorizontal = tangentVertical * (std::max)(previewAspect, 0.1f);
        float requiredDistance = 0.1f;
        for (int corner = 0; corner < 8; corner++)
        {
            const float localX = (corner & 1) ? maximum.x : minimum.x;
            const float localY = (corner & 2) ? maximum.y : minimum.y;
            const float localZ = (corner & 4) ? maximum.z : minimum.z;
            const float relativeX = localX - boundsCenter.x;
            const float relativeY = localY - boundsCenter.y;
            const float relativeZ = localZ - boundsCenter.z;
            const float rotatedX = relativeX * cosine - relativeY * sine;
            const float rotatedY = relativeX * sine + relativeY * cosine;

            const float projectedRight = rotatedX * rightX + rotatedY * rightY;
            const float projectedUp = rotatedX * upX + rotatedY * upY + relativeZ * upZ;
            const float projectedDepth = rotatedX * directionX + rotatedY * directionY + relativeZ * directionZ;
            const float horizontalDistance = projectedDepth + std::fabs(projectedRight) / tangentHorizontal;
            const float verticalDistance = projectedDepth + std::fabs(projectedUp) / tangentVertical;
            requiredDistance = (std::max)(requiredDistance, (std::max)(horizontalDistance, verticalDistance));
        }
        distance = (std::max)(distance, requiredDistance * kPreviewFitMargin);
    }
    if (preview.GetRootType() == EntityType::VEHICLE)
        distance *= kVehiclePreviewDistanceMultiplier;
    if (preview.GetRootType() == EntityType::PED)
        distance = (std::max)(distance, kPedPreviewCameraDistance);
    patch.pos[0] = directionX * distance;
    patch.pos[1] = directionY * distance;
    patch.pos[2] = directionZ * distance;
    patch.rot[0] = 0.0f;
    patch.rot[1] = 0.0f;
    patch.rot[2] = g_rotationDegrees;
    patch.rect[0] = (centerX - width * 0.5f + (aspectScale - 1.0f) * 0.5f) / aspectScale;
    patch.rect[1] = centerY - height * 0.5f;
    patch.rect[2] = width / aspectScale;
    patch.rect[3] = height;
    patch.bgColour = kPreviewBackground;
    patch.lightIntensity = 1.0f;

    if (!UI3DScene::PatchPresetElement(kPreviewPreset, kPreviewElement, patch))
        return false;

    UI3DScene::SetCameraFov(cameraFov);
    UI3DScene::SetLightRigScale(1.0f, 1.0f);
    return true;
}

bool DrawActivePreview(float centerX, float centerY, float width, float height)
{
    if (!g_activePreview)
        return false;
    g_lastDrawTimeMilliseconds = static_cast<DWORD>(GET_GAME_TIMER());
    void* rootEntity = g_activePreview->GetRootAddress();
    if (!rootEntity)
    {
        g_activePreview->Tick();
        return false;
    }

    UI3DScene::ElementPatch patch{};
    if (!SetPreviewViewport(*g_activePreview, centerX, centerY, width, height, patch))
    {
        LogPreviewFailureOnce("failed to set preview viewport");
        return false;
    }
    g_rotationDegrees = std::fmod(g_rotationDegrees + 20.0f * GET_FRAME_TIME(), 360.0f);

    UI3DScene::SetDefaultDrawParams(255, g_activePreview->GetRootType() == EntityType::PROP ? 1 : 0);
    if (!UI3DScene::SetPreviewEntity(rootEntity))
    {
        LogPreviewFailureOnce("UI3DSCENE draw handler is not ready");
        return false;
    }

    const Vector3 entityPosition(patch.pos[0], patch.pos[1], patch.pos[2]);
    g_activePreview->PlaceAt(entityPosition, g_rotationDegrees);
    std::vector<UI3DScene::ExtraEntity> extraEntities;
    g_activePreview->GetExtraEntities(extraEntities);
    UI3DScene::SetPreviewExtraEntities(extraEntities.data(), static_cast<int>(extraEntities.size()));

    if (!UI3DScene::PushPreset(kPreviewPreset) && !UI3DScene::IsPresetActive(kPreviewPreset))
    {
        LogPreviewFailureOnce("failed to activate UI3DSCENE preset");
        return false;
    }

    const float zeroPosition[3] = {0.0f, 0.0f, 0.0f};
    const float sceneRotation[3] = {patch.rot[0], patch.rot[1], g_rotationDegrees};
    if (!UI3DScene::AssignEntityToSlot(kPreviewPreset, kPreviewSlot, rootEntity, zeroPosition, sceneRotation))
    {
        LogPreviewFailureOnce("failed to assign entity to UI3DSCENE slot");
        return false;
    }
    return true;
}
} // namespace

bool g_enabled = true;

bool DrawModelPreview(const std::string& previewKey, EntityType entityType, const GTAmodel::Model& model, float centerX, float centerY, float width, float height, const std::string& previewDataPath)
{
    if (!g_enabled || !UI3DScene::IsReady() || IS_PAUSE_MENU_ACTIVE() || previewKey.empty() || model.hash == 0)
        return false;

    const std::string effectivePreviewKey = entityType == EntityType::VEHICLE ? "vehicle:" + std::to_string(model.hash) : previewKey;
    if (g_activePreviewKey != effectivePreviewKey || g_activeEntityType != entityType || g_activeModelHash != model.hash || !g_activePreview)
    {
        Clear();
        g_activePreview = CreatePreview(entityType, model, previewDataPath);
        if (!g_activePreview)
            return false;
        g_activePreviewKey = effectivePreviewKey;
        g_activeEntityType = entityType;
        g_activeModelHash = model.hash;
    }

    return DrawActivePreview(centerX, centerY, width, height);
}

bool DrawPedAnimationPreview(const GTAmodel::Model& model, int sourcePedHandle, const PedPreview3D::Animation& animation, float centerX, float centerY, float width, float height)
{
    if (!g_enabled || !UI3DScene::IsReady() || IS_PAUSE_MENU_ACTIVE() || model.hash == 0 || animation.dict.empty() || animation.name.empty())
        return false;

    const std::string previewKey = "animation-preview:" + std::to_string(sourcePedHandle) + ":" + std::to_string(model.hash);
    if (g_activePreviewKey != previewKey || g_activeEntityType != EntityType::PED || g_activeModelHash != model.hash || !g_activePreview)
    {
        Clear();
        g_activePreview = std::make_unique<PedPreview3D>(model, "", sourcePedHandle);
        g_activePreviewKey = previewKey;
        g_activeEntityType = EntityType::PED;
        g_activeModelHash = model.hash;
    }

    auto* pedPreview = dynamic_cast<PedPreview3D*>(g_activePreview.get());
    if (!pedPreview)
        return false;

    pedPreview->SetAnimation(animation);
    return DrawActivePreview(centerX, centerY, width, height);
}

bool DrawSpoonerScenePreview(const std::string& filePath, float centerX, float centerY, float width, float height)
{
    if (!g_enabled || !UI3DScene::IsReady() || IS_PAUSE_MENU_ACTIVE() || filePath.empty())
        return false;

    const std::string previewKey = "spooner-scene:" + filePath;
    if (g_activePreviewKey != previewKey || !g_activePreview)
    {
        Clear();
        auto preview = std::make_unique<SpoonerScenePreview3D>(filePath);
        if (!preview->IsValid())
            return false;
        g_activePreview = std::move(preview);
        g_activePreviewKey = previewKey;
        g_activeEntityType = EntityType::ALL;
    }

    return DrawActivePreview(centerX, centerY, width, height);
}

StatsPanel::Preview3DRow CreateModelPreviewRow(const std::string& previewKey, EntityType entityType, const GTAmodel::Model& model, const std::string& previewDataPath)
{
    constexpr float previewWidth = 0.1f;
    float targetAspectRatio = 2.0f;
    if (entityType == EntityType::PED)
        targetAspectRatio = 0.72f;
    else if (entityType == EntityType::PROP)
        targetAspectRatio = 1.0f;
    const float screenAspectRatio = GET_ASPECT_RATIO(false);
    const float previewHeight = screenAspectRatio > 0.0f ? previewWidth * screenAspectRatio / targetAspectRatio : previewWidth * (16.0f / 9.0f) / targetAspectRatio;

    return StatsPanel::Preview3DRow{
        [previewKey, entityType, model, previewDataPath](float centerX, float centerY, float width, float height)
        { return DrawModelPreview(previewKey, entityType, model, centerX, centerY, width, height, previewDataPath); },
        previewWidth,
        previewHeight,
        0.006f
    };
}

StatsPanel::Preview3DRow CreatePedAnimationPreviewRow(const GTAmodel::Model& model, int sourcePedHandle, const PedPreview3D::Animation& animation)
{
    constexpr float previewWidth = 0.1f;
    constexpr float targetAspectRatio = 0.72f;
    const float screenAspectRatio = GET_ASPECT_RATIO(false);
    const float previewHeight = screenAspectRatio > 0.0f ? previewWidth * screenAspectRatio / targetAspectRatio : previewWidth * (16.0f / 9.0f) / targetAspectRatio;

    return StatsPanel::Preview3DRow{
        [model, sourcePedHandle, animation](float centerX, float centerY, float width, float height)
        { return DrawPedAnimationPreview(model, sourcePedHandle, animation, centerX, centerY, width, height); },
        previewWidth,
        previewHeight,
        0.006f
    };
}

void Tick()
{
    if (!g_enabled)
    {
        Clear();
        return;
    }

    if (!g_activePreview)
        return;

    const DWORD currentTimeMilliseconds = static_cast<DWORD>(GET_GAME_TIMER());
    if (g_lastDrawTimeMilliseconds == 0 || currentTimeMilliseconds - g_lastDrawTimeMilliseconds > kPreviewInactiveTimeoutMilliseconds)
    {
        Clear();
        return;
    }

    g_activePreview->Tick();
}

void Clear()
{
    UI3DScene::SetPreviewEntity(nullptr);
    UI3DScene::SetDefaultDrawParams(255, 0);
    UI3DScene::RestorePresetPatches();
    UI3DScene::SetCameraFov(0.0f);
    g_activePreview.reset();
    g_activePreviewKey.clear();
    g_activeEntityType = EntityType::ALL;
    g_activeModelHash = 0;
    g_lastDrawTimeMilliseconds = 0;
    g_previewFailureLogged = false;
}

void AbandonForProcessExit()
{
    g_activePreview.release();
}
} // namespace Previews
