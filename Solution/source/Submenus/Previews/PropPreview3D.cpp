/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "PropPreview3D.h"

#include "..\..\Natives\natives2.h"

namespace Previews
{
PropPreview3D::PropPreview3D(const GTAmodel::Model& model) : ModelPreview3D(EntityType::PROP, model)
{
}

int PropPreview3D::CreateEntity(Hash modelHash, const Vector3& spawnPosition, float headingDegrees)
{
    const int object = CREATE_OBJECT(modelHash, spawnPosition.x, spawnPosition.y, spawnPosition.z, 0, 0, 1);
    SET_ENTITY_HEADING(object, headingDegrees);
    return object;
}
} // namespace Previews
