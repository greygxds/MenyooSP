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
#include "..\Spooner\SpoonerEntity.h"

#include <string>
#include <vector>

namespace Previews
{
class PedPreview3D final : public ModelPreview3D
{
  public:
    using Animation = sub::Spooner::SpoonerEntity::Animation;

    explicit PedPreview3D(const GTAmodel::Model& model, std::string outfitFilePath = "", int sourcePedHandle = 0);
    void SetAnimation(const Animation& animation);
    void SetAnimations(std::vector<Animation> animations);
    void Tick() override;

  protected:
    int CreateEntity(Hash modelHash, const Vector3& spawnPosition, float headingDegrees) override;
    void ConfigureEntity(int entityHandle) override;

  private:
    std::string outfitFilePath;
    int sourcePedHandle = 0;
    std::vector<Animation> animations;
    std::vector<bool> animationStarted;
    bool animationTasksCleared = false;
};
} // namespace Previews
