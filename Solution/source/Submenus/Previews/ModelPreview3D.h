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

#include "Preview3DContent.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\Model.h"

namespace Previews
{
void GetModelPreviewBounds(const GTAmodel::Model& model, Vector3& minimum, Vector3& maximum);

class ModelPreview3D : public Preview3DContent
{
  public:
    ModelPreview3D(EntityType entityType, const GTAmodel::Model& model);
    virtual ~ModelPreview3D();

    ModelPreview3D(const ModelPreview3D&) = delete;
    ModelPreview3D& operator=(const ModelPreview3D&) = delete;

    void Tick();
    int GetEntityHandle() const;
    EntityType GetEntityType() const;
    EntityType GetRootType() const override;
    void* GetRootAddress() const override;
    const GTAmodel::Model& GetFramingModel() const override;
    float GetSceneRadius() const override;
    void PlaceAt(const Vector3& center, float headingDegrees) override;
    void GetExtraEntities(std::vector<UI3DScene::ExtraEntity>& extraEntities) const override;

  protected:
    virtual int CreateEntity(Hash modelHash, const Vector3& spawnPosition, float headingDegrees) = 0;
    virtual void ConfigureEntity(int entityHandle);

  private:
    GTAmodel::Model model;
    GTAentity previewEntity;
    EntityType entityType;
    DWORD modelLoadRequestedAt = 0;
    int entityCreatedFrame = -1;
    bool modelLoadDelayLogged = false;
    bool entityCreationFailureLogged = false;
    mutable bool entityAddressFailureLogged = false;
};
} // namespace Previews
