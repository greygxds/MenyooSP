/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#include "WardrobeCamera.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Menu\submenu_enum.h"
#include "..\..\Misc\FreeCam.h"
#include "..\..\Scripting\Camera.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\World.h"
#include "..\..\Submenus\Spooner\SpoonerMode.h"
#include "..\..\Util\GTAmath.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

namespace sub
{ // Wardrobe front view camera

namespace WardrobeCamera
{
namespace
{
constexpr int cameraTransitionDurationMs = 1000;
constexpr int cameraExitTransitionDurationMs = 900;
constexpr DWORD cameraTakeoverGraceBufferMs = 500;
constexpr DWORD gameplayExitCleanupBufferMs = 150;
constexpr DWORD cameraTakeoverGracePeriodMs = cameraTransitionDurationMs + cameraTakeoverGraceBufferMs;
constexpr DWORD gameplayCameraExitCleanupDelayMs = cameraExitTransitionDurationMs + gameplayExitCleanupBufferMs;
constexpr float cameraFieldOfViewDegrees = 40.0f;
constexpr float headCameraForwardOffsetMeters = 0.6f;
constexpr float bodyCameraForwardOffsetMeters = 2.6f;
constexpr float bodyCameraVerticalOffsetMeters = 0.5f;

Camera activeCamera;
GTAped cameraTargetPed;
Framing currentFraming = Framing::Body;
DWORD cameraEnabledAtTick = 0;

struct ExitTransitionState
{
    Camera transitionCamera;
    Camera targetCamera;
    bool exitsToScriptCamera = false;
    DWORD startedAtTick = 0;
    Vector3 startPosition{};
    Vector3 startRotation{};
    float startFieldOfView = 0.0f;
};

ExitTransitionState exitTransition;

bool IsPedWardrobeSubmenu(int submenuId)
{
    switch (submenuId)
    {
    case SUB::PED_WARDROBE:
    case SUB::PED_COMPONENT_VARIATIONS:
    case SUB::PED_OUTFITS:
    case SUB::PED_OUTFIT_OPTIONS:
    case SUB::PED_DEFAULT_OUTFITS:
    case SUB::PED_PROPS:
    case SUB::PED_PROP_VARIATIONS:
    case SUB::PED_VARIATION_COLLECTIONS:
    case SUB::PED_DECALS_TYPES:
    case SUB::PED_DECALS_ZONES:
    case SUB::PED_DECALS_ZONE_SEARCH:
    case SUB::PED_DECALS_IN_ZONE:
    case SUB::PED_DAMAGE_CATEGORY_LIST:
    case SUB::PED_DAMAGE_BONE_SELECTION:
    case SUB::PED_DAMAGE_BLOOD:
    case SUB::PED_DAMAGE_DECALS:
    case SUB::PED_DAMAGE_PACKS:
    case SUB::PED_HEAD:
    case SUB::PED_HEAD_OVERLAYS:
    case SUB::PED_HEAD_OVERLAY_SETTINGS:
    case SUB::PED_HEAD_FACE_FEATURES:
    case SUB::PED_HEAD_FACE_GENERATOR:
    case SUB::PED_HEAD_SKIN_TONE:
        return true;
    default:
        return false;
    }
}

void ApplyCurrentFraming()
{
    if (!activeCamera.Exists() || !cameraTargetPed.Exists())
        return;

    if (currentFraming == Framing::Head)
    {
        activeCamera.AttachTo(cameraTargetPed, Bone::Head, Vector3(0.0f, headCameraForwardOffsetMeters, 0.0f));
        activeCamera.PointAt(cameraTargetPed, Bone::Head);
    }
    else
    {
        activeCamera.AttachTo(cameraTargetPed, Vector3(0.0f, bodyCameraForwardOffsetMeters + cameraTargetPed.Dim1().y, bodyCameraVerticalOffsetMeters));
        activeCamera.PointAt(cameraTargetPed);
    }
}

// The camera that should render once the wardrobe camera exits.
Camera GetCameraToRestore()
{
    if (Spooner::SpoonerMode::bEnabled && Spooner::SpoonerCamera::camera.Exists())
        return Spooner::SpoonerCamera::camera;
    if (FreeCamMode::IsActive() && FreeCamMode::GetCamera().Exists())
        return FreeCamMode::GetCamera();
    return Camera();
}

void DestroyCamera(Camera& cameraToDestroy)
{
    if (cameraToDestroy.Exists())
    {
        cameraToDestroy.SetActive(false);
        cameraToDestroy.Destroy();
    }
    cameraToDestroy = Camera();
}

void FinishExitTransition()
{
    DestroyCamera(exitTransition.transitionCamera);
    exitTransition.targetCamera = Camera();
}

float WrapAngleDegrees(float angleDegrees)
{
    angleDegrees = fmod(angleDegrees + 180.0f, 360.0f);
    return angleDegrees < 0.0f ? angleDegrees + 180.0f : angleDegrees - 180.0f;
}

void UpdateScriptCameraExitTransition(DWORD exitElapsedMs)
{
    if (!exitTransition.targetCamera.Exists())
    {
        FinishExitTransition();
        return;
    }

    const float transitionProgress = (std::min)(1.0f, static_cast<float>(exitElapsedMs) / cameraExitTransitionDurationMs);
    const float easedProgress = transitionProgress * transitionProgress * (3.0f - 2.0f * transitionProgress);

    const Vector3 targetPosition = exitTransition.targetCamera.GetPosition();
    const Vector3 targetRotation = exitTransition.targetCamera.GetRotation();
    const float targetFieldOfView = exitTransition.targetCamera.GetFieldOfView();

    exitTransition.transitionCamera.SetPosition(Vector3(
        exitTransition.startPosition.x + (targetPosition.x - exitTransition.startPosition.x) * easedProgress,
        exitTransition.startPosition.y + (targetPosition.y - exitTransition.startPosition.y) * easedProgress,
        exitTransition.startPosition.z + (targetPosition.z - exitTransition.startPosition.z) * easedProgress
    ));
    exitTransition.transitionCamera.SetRotation(Vector3(
        exitTransition.startRotation.x + WrapAngleDegrees(targetRotation.x - exitTransition.startRotation.x) * easedProgress,
        exitTransition.startRotation.y + WrapAngleDegrees(targetRotation.y - exitTransition.startRotation.y) * easedProgress,
        exitTransition.startRotation.z + WrapAngleDegrees(targetRotation.z - exitTransition.startRotation.z) * easedProgress
    ));
    exitTransition.transitionCamera.SetFieldOfView(exitTransition.startFieldOfView + (targetFieldOfView - exitTransition.startFieldOfView) * easedProgress);

    if (transitionProgress >= 1.0f)
    {
        // both cameras share the same pose now, so this switch is invisible
        World::SetRenderingCamera(exitTransition.targetCamera);
        FinishExitTransition();
    }
}

void TickExitTransition()
{
    if (exitTransition.transitionCamera.Handle() == 0)
        return;
    if (!exitTransition.transitionCamera.Exists())
    {
        FinishExitTransition();
        return;
    }

    const DWORD exitElapsedMs = GetTickCount() - exitTransition.startedAtTick;
    if (exitTransition.exitsToScriptCamera)
    {
        UpdateScriptCameraExitTransition(exitElapsedMs);
        return;
    }

    if (exitElapsedMs > gameplayCameraExitCleanupDelayMs)
        FinishExitTransition();
}

void BeginExitTransition()
{
    FinishExitTransition();

    exitTransition.transitionCamera = activeCamera;
    activeCamera = Camera();
    cameraTargetPed = GTAped();
    exitTransition.startedAtTick = GetTickCount();

    // freeze the shot where it is so the blend doesn't follow the ped around
    exitTransition.transitionCamera.Detach();
    exitTransition.transitionCamera.StopPointing();
    exitTransition.startPosition = exitTransition.transitionCamera.GetPosition();
    exitTransition.startRotation = exitTransition.transitionCamera.GetRotation();
    exitTransition.startFieldOfView = exitTransition.transitionCamera.GetFieldOfView();

    exitTransition.targetCamera = GetCameraToRestore();
    exitTransition.exitsToScriptCamera = exitTransition.targetCamera.Exists();
    if (!exitTransition.exitsToScriptCamera)
        Camera::RenderScriptCams(false, true, cameraExitTransitionDurationMs);
}
} // namespace

bool IsActive()
{
    return activeCamera.Exists();
}

bool IsBusy()
{
    return IsActive() || exitTransition.transitionCamera.Exists();
}

void Enable(const GTAped& targetPed)
{
    if (IsActive() || !targetPed.Exists())
        return;

    // re-entering mid ease-out: jump to the view we were easing to, then blend in from there
    if (exitTransition.exitsToScriptCamera && exitTransition.targetCamera.Exists() && exitTransition.transitionCamera.Exists())
        World::SetRenderingCamera(exitTransition.targetCamera);
    FinishExitTransition();
    Camera previousCamera = World::GetRenderingCamera();

    activeCamera = World::CreateCamera(false);
    activeCamera.SetFieldOfView(cameraFieldOfViewDegrees);
    cameraTargetPed = targetPed;
    currentFraming = Framing::Body;
    ApplyCurrentFraming();
    cameraEnabledAtTick = GetTickCount();

    if (previousCamera.Exists())
        previousCamera.InterpTo(activeCamera, cameraTransitionDurationMs, true, true);
    else
    {
        activeCamera.SetActive(true);
        Camera::RenderScriptCams(true, true, cameraTransitionDurationMs);
    }
}

void Disable(bool restorePreviousView)
{
    if (restorePreviousView && activeCamera.Exists())
    {
        BeginExitTransition();
        return;
    }

    // hard hand-off: another camera is taking over, so cut immediately
    FinishExitTransition();
    DestroyCamera(activeCamera);
    cameraTargetPed = GTAped();
}

void SetFraming(const GTAped& targetPed, Framing newFraming)
{
    if (!IsActive())
        return;

    cameraTargetPed = targetPed;
    currentFraming = newFraming;
    ApplyCurrentFraming();
}

void Tick()
{
    TickExitTransition();
    if (activeCamera.Handle() == 0)
        return;

    // destroyed externally (e.g. "Delete All Cameras")
    if (!activeCamera.Exists())
    {
        Disable(false);
        return;
    }

    if (!IsPedWardrobeSubmenu(Menu::activeSubmenu) || !cameraTargetPed.Exists())
    {
        Disable();
        return;
    }

    // another camera (Spooner, FreeCam, ...) took over the view
    if (GetTickCount() - cameraEnabledAtTick > cameraTakeoverGracePeriodMs && !activeCamera.IsInterpolating() && World::GetRenderingCamera().Handle() != activeCamera.Handle())
    {
        Disable(false);
    }
}
} // namespace WardrobeCamera
} // namespace sub
