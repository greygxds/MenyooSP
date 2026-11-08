/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "ModelPreview3D.h"

#include "..\..\Natives\natives2.h"
#include "..\..\Util\FileLogger.h"

#include <cmath>
#include <Windows.h>

namespace Previews
{
void GetModelPreviewBounds(const GTAmodel::Model& model, Vector3& minimum, Vector3& maximum)
{
    const GTAmodel::ModelDimensions dimensions = model.Dimensions();
    minimum = Vector3(-dimensions.Dim2.x, -dimensions.Dim2.y, -dimensions.Dim1.z);
    maximum = Vector3(dimensions.Dim1.x, dimensions.Dim1.y, dimensions.Dim2.z);
}

ModelPreview3D::ModelPreview3D(EntityType previewEntityType, const GTAmodel::Model& previewModel) : model(previewModel), previewEntity(0), entityType(previewEntityType)
{
    modelLoadRequestedAt = GetTickCount();
}

ModelPreview3D::~ModelPreview3D()
{
    int entityHandle = previewEntity.GetHandle();
    if (entityHandle != 0 && DOES_ENTITY_EXIST(entityHandle))
        previewEntity.Delete(true);

    if (model.hash != 0 && model.IsLoaded())
        model.Unload();
}

void ModelPreview3D::Tick()
{
    if (previewEntity.GetHandle() != 0)
        return;

    if (!model.IsValid() || !model.IsInCdImage())
    {
        if (!entityCreationFailureLogged)
        {
            addlog(ige::LogType::LOG_WARNING, "Preview3D: invalid or unavailable model hash " + std::to_string(model.hash));
            entityCreationFailureLogged = true;
        }
        return;
    }

    model.Load();
    if (!model.IsLoaded())
    {
        if (!modelLoadDelayLogged && GetTickCount() - modelLoadRequestedAt > 10000)
        {
            addlog(ige::LogType::LOG_WARNING, "Preview3D: model load still pending for hash " + std::to_string(model.hash));
            modelLoadDelayLogged = true;
        }
        return;
    }

    Vector3 spawnPosition = GET_ENTITY_COORDS(PLAYER_PED_ID(), false);
    spawnPosition.z += 40.0f;
    const float spawnHeading = GET_ENTITY_HEADING(PLAYER_PED_ID());
    const int createdEntity = CreateEntity(model.hash, spawnPosition, spawnHeading);
    if (createdEntity == 0)
    {
        if (!entityCreationFailureLogged)
        {
            addlog(ige::LogType::LOG_ERROR, "Preview3D: entity creation failed for model hash " + std::to_string(model.hash));
            entityCreationFailureLogged = true;
        }
        return;
    }

    previewEntity = createdEntity;
    entityCreatedFrame = GET_FRAME_COUNT();
    SET_ENTITY_AS_MISSION_ENTITY(createdEntity, true, true);
    SET_ENTITY_ALPHA(createdEntity, 0, false);
    SET_ENTITY_COLLISION(createdEntity, false, false);
    FREEZE_ENTITY_POSITION(createdEntity, true);
    SET_ENTITY_INVINCIBLE(createdEntity, true);
    ConfigureEntity(createdEntity);
}

int ModelPreview3D::GetEntityHandle() const
{
    return previewEntity.GetHandle();
}

EntityType ModelPreview3D::GetEntityType() const
{
    return entityType;
}

EntityType ModelPreview3D::GetRootType() const
{
    return entityType;
}

void* ModelPreview3D::GetRootAddress() const
{
    if (previewEntity.GetHandle() == 0 || GET_FRAME_COUNT() - entityCreatedFrame < 3)
        return nullptr;
    const UINT64 entityAddress = previewEntity.MemoryAddress();
    if (entityAddress == 0)
    {
        if (!entityAddressFailureLogged && GET_FRAME_COUNT() - entityCreatedFrame >= 60)
        {
            addlog(ige::LogType::LOG_ERROR, "Preview3D: entity memory lookup failed for handle " + std::to_string(previewEntity.GetHandle()) + ", model hash " + std::to_string(model.hash));
            entityAddressFailureLogged = true;
        }
        return nullptr;
    }
    return reinterpret_cast<void*>(entityAddress);
}

const GTAmodel::Model& ModelPreview3D::GetFramingModel() const
{
    return model;
}

float ModelPreview3D::GetSceneRadius() const
{
    Vector3 minimum;
    Vector3 maximum;
    GetModelPreviewBounds(model, minimum, maximum);
    const Vector3 dimensions = maximum - minimum;
    return std::sqrt(dimensions.x * dimensions.x + dimensions.y * dimensions.y + dimensions.z * dimensions.z) * 0.5f;
}

void ModelPreview3D::PlaceAt(const Vector3& center, float headingDegrees)
{
    if (previewEntity.GetHandle() == 0)
        return;

    Vector3 minimum;
    Vector3 maximum;
    GetModelPreviewBounds(model, minimum, maximum);
    const Vector3 boundsCenter = (minimum + maximum) * 0.5f;
    const float headingRadians = headingDegrees * 0.017453292f;
    const float cosine = std::cos(headingRadians);
    const float sine = std::sin(headingRadians);
    const Vector3 rotatedBoundsCenter(boundsCenter.x * cosine - boundsCenter.y * sine, boundsCenter.x * sine + boundsCenter.y * cosine, boundsCenter.z);
    previewEntity.SetPosition(center - rotatedBoundsCenter);
    previewEntity.SetRotation(Vector3(0.0f, 0.0f, headingDegrees));
    RESET_ENTITY_ALPHA(previewEntity.GetHandle());
}

void ModelPreview3D::GetExtraEntities(std::vector<UI3DScene::ExtraEntity>&) const
{
}

void ModelPreview3D::ConfigureEntity(int)
{
}
} // namespace Previews
