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

#include "..\..\Memory\UI3DScene.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\Model.h"

#include <vector>

namespace Previews
{
class Preview3DContent
{
  public:
    virtual ~Preview3DContent() = default;
    virtual void Tick() = 0;
    virtual EntityType GetRootType() const = 0;
    virtual void* GetRootAddress() const = 0;
    virtual const GTAmodel::Model& GetFramingModel() const = 0;
    virtual float GetSceneRadius() const = 0;

    virtual bool IsCompositeScene() const
    {
        return false;
    }

    virtual void PlaceAt(const Vector3& center, float headingDegrees) = 0;
    virtual void GetExtraEntities(std::vector<UI3DScene::ExtraEntity>& extraEntities) const = 0;
};
} // namespace Previews
