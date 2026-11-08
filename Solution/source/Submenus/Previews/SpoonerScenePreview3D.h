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

#include <memory>
#include <string>
#include <vector>

namespace Previews
{
class ModelPreview3D;

class SpoonerScenePreview3D final : public Preview3DContent
{
  public:
    explicit SpoonerScenePreview3D(const std::string& filePath);
    ~SpoonerScenePreview3D() override;

    void Tick() override;
    EntityType GetRootType() const override;
    void* GetRootAddress() const override;
    const GTAmodel::Model& GetFramingModel() const override;
    float GetSceneRadius() const override;

    bool IsCompositeScene() const override
    {
        return true;
    }

    void PlaceAt(const Vector3& center, float headingDegrees) override;
    void GetExtraEntities(std::vector<UI3DScene::ExtraEntity>& extraEntities) const override;
    bool IsValid() const;

  private:
    struct Placement
    {
        EntityType type = EntityType::ALL;
        Vector3 position;
        float headingDegrees = 0.0f;
        std::unique_ptr<ModelPreview3D> preview;
        std::string pedPropertiesXml;
        bool pedAppearanceApplied = false;
    };

    std::vector<Placement> placements;
    std::size_t rootIndex = 0;
    Vector3 sceneCenter;
    float sceneRadius = 0.0f;
    float sceneScale = 1.0f;
    std::size_t nextPlacementIndex = 0;
};
} // namespace Previews
