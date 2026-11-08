/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "PedPreview3D.h"

#include "..\..\Natives\natives2.h"
#include "..\Wardrobe\PedOutfitPersistence.h"
#include "..\..\Scripting\GTAped.h"

#include <utility>

namespace Previews
{
PedPreview3D::PedPreview3D(const GTAmodel::Model& model, std::string outfitFilePath, int sourcePedHandle)
    : ModelPreview3D(EntityType::PED, model), outfitFilePath(std::move(outfitFilePath)), sourcePedHandle(sourcePedHandle)
{
}

void PedPreview3D::SetAnimation(const Animation& newAnimation)
{
    if (animations.size() == 1 && animations.front().dict == newAnimation.dict && animations.front().name == newAnimation.name && animations.front().speed == newAnimation.speed &&
        animations.front().speedMultiplier == newAnimation.speedMultiplier && animations.front().duration == newAnimation.duration && animations.front().flag == newAnimation.flag &&
        animations.front().playbackRate == newAnimation.playbackRate && animations.front().lockPos == newAnimation.lockPos)
        return;

    SetAnimations({newAnimation});
}

void PedPreview3D::SetAnimations(std::vector<Animation> newAnimations)
{
    animations = std::move(newAnimations);
    animationStarted.assign(animations.size(), false);
    animationTasksCleared = false;
}

void PedPreview3D::Tick()
{
    ModelPreview3D::Tick();

    if (GetEntityHandle() == 0)
        return;

    for (std::size_t index = 0; index < animations.size(); index++)
    {
        const auto& animation = animations[index];
        if (animationStarted[index] || animation.dict.empty() || animation.name.empty())
            continue;

        REQUEST_ANIM_DICT(animation.dict.c_str());
        if (!HAS_ANIM_DICT_LOADED(animation.dict.c_str()))
            continue;

        if (!animationTasksCleared)
        {
            CLEAR_PED_TASKS_IMMEDIATELY(GetEntityHandle());
            animationTasksCleared = true;
        }
        TASK_PLAY_ANIM(
            GetEntityHandle(),
            animation.dict.c_str(),
            animation.name.c_str(),
            animation.speed,
            animation.speedMultiplier,
            animation.duration,
            animation.flag,
            animation.playbackRate,
            animation.lockPos,
            animation.lockPos,
            animation.lockPos
        );
        animationStarted[index] = true;
    }
}

int PedPreview3D::CreateEntity(Hash modelHash, const Vector3& spawnPosition, float headingDegrees)
{
    if (sourcePedHandle != 0 && DOES_ENTITY_EXIST(sourcePedHandle) && IS_ENTITY_A_PED(sourcePedHandle))
    {
        const int clonedPed = CLONE_PED(sourcePedHandle, false, true, true);
        if (clonedPed != 0)
        {
            SET_ENTITY_COORDS(clonedPed, spawnPosition.x, spawnPosition.y, spawnPosition.z, false, false, false, true);
            SET_ENTITY_HEADING(clonedPed, headingDegrees);
            return clonedPed;
        }
    }

    return CREATE_PED(26, modelHash, spawnPosition.x, spawnPosition.y, spawnPosition.z, headingDegrees, 1, 1);
}

void PedPreview3D::ConfigureEntity(int entityHandle)
{
    SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(entityHandle, true);
    SET_PED_CAN_RAGDOLL(entityHandle, false);
    CLEAR_PED_TASKS_IMMEDIATELY(entityHandle);
    if (!outfitFilePath.empty())
        sub::PedOutfitPersistence::ApplyPedOutfit(GTAped(entityHandle), outfitFilePath, sub::PedOutfitPersistence::OutfitApplyOptions::CompleteWithoutAttachments());
}
} // namespace Previews
