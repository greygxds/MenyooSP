/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "VehiclePreview3D.h"

#include "..\..\Natives\natives2.h"

namespace Previews
{
VehiclePreview3D::VehiclePreview3D(const GTAmodel::Model& model) : ModelPreview3D(EntityType::VEHICLE, model)
{
}

int VehiclePreview3D::CreateEntity(Hash modelHash, const Vector3& spawnPosition, float headingDegrees)
{
    return CREATE_VEHICLE(modelHash, spawnPosition.x, spawnPosition.y, spawnPosition.z, headingDegrees, 0, 0, 0);
}

void VehiclePreview3D::ConfigureEntity(int entityHandle)
{
    SET_VEHICLE_ENGINE_ON(entityHandle, false, true, false);
    SET_VEHICLE_DOORS_LOCKED(entityHandle, 2);
}
} // namespace Previews
