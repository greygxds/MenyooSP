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

#include "ModelPreview3D.h"

namespace Previews
{
class PropPreview3D final : public ModelPreview3D
{
  public:
    explicit PropPreview3D(const GTAmodel::Model& model);

  protected:
    int CreateEntity(Hash modelHash, const Vector3& spawnPosition, float headingDegrees) override;
};
} // namespace Previews
