/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "SpoonerScenePreview3D.h"

#include "ModelPreview3D.h"
#include "PedPreview3D.h"
#include "PropPreview3D.h"
#include "VehiclePreview3D.h"

#include "..\..\Natives\natives2.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\enums.h"
#include "..\Spooner\FileManagement.h"

#include <pugixml\src\pugixml.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>

namespace Previews
{
namespace
{
constexpr std::size_t kMaximumPreviewPlacements = 16;
constexpr float kMaximumSceneRadius = 3.5f;

std::unique_ptr<ModelPreview3D> CreatePlacementPreview(EntityType entityType, const GTAmodel::Model& model)
{
    switch (entityType)
    {
    case EntityType::PED:
        return std::make_unique<PedPreview3D>(model);
    case EntityType::VEHICLE:
        return std::make_unique<VehiclePreview3D>(model);
    case EntityType::PROP:
        return std::make_unique<PropPreview3D>(model);
    default:
        return nullptr;
    }
}

std::vector<PedPreview3D::Animation> ReadPedAnimations(const pugi::xml_node& placementNode)
{
    std::vector<PedPreview3D::Animation> animations;
    const auto pedProperties = placementNode.child("PedProperties");
    const auto animationNodes = pedProperties.child("Animations");
    for (auto node = animationNodes.child("Animation"); node; node = node.next_sibling("Animation"))
    {
        PedPreview3D::Animation animation;
        animation.dict = node.child("Dict").text().as_string();
        animation.name = node.child("Name").text().as_string();
        animation.speed = node.child("Speed").text().as_float(4.0f);
        animation.speedMultiplier = node.child("SpeedMultiplier").text().as_float(-4.0f);
        animation.playbackRate = node.child("PlaybackRate").text().as_float(0.0f);
        animation.duration = node.child("Duration").text().as_int(-1);
        animation.flag = node.child("Flag").text().as_int(1);
        animation.lockPos = node.child("LockPos").text().as_bool(false);
        if (!animation.dict.empty() && !animation.name.empty())
            animations.push_back(std::move(animation));
    }

    if (animations.empty() && pedProperties.child("AnimActive").text().as_bool())
    {
        PedPreview3D::Animation animation;
        animation.dict = pedProperties.child("AnimDict").text().as_string();
        animation.name = pedProperties.child("AnimName").text().as_string();
        if (!animation.dict.empty() && !animation.name.empty())
            animations.push_back(std::move(animation));
    }

    return animations;
}

void ApplyPedAppearance(GTAped ped, const std::string& pedPropertiesXml, const GTAmodel::Model& model)
{
    pugi::xml_document document;
    if (document.load_string(pedPropertiesXml.c_str()).status != pugi::status_ok)
        return;

    const auto pedProperties = document.child("PedProperties");
    sub::Spooner::FileManagement::LoadPedCompsFromXml(ped, pedProperties.child("PedComps"), false);
    sub::Spooner::FileManagement::LoadPedPropsFromXml(ped, pedProperties.child("PedProps"), NETWORK_IS_GAME_IN_PROGRESS() != 0, false);
    sub::Spooner::FileManagement::LoadPedHeadFeaturesFromXml(ped, pedProperties.child("HeadFeatures"), model);
    sub::Spooner::FileManagement::LoadPedDecalsFromXml(ped, pedProperties.child("TattooLogoDecals"));
    sub::Spooner::FileManagement::LoadPedDamagePacksFromXml(ped, pedProperties.child("DamagePacks"));
    if (pedProperties.child("HasShortHeight").text().as_bool())
        SET_PED_CONFIG_FLAG(ped.Handle(), ePedConfigFlags::_Shrink, true);
}
} // namespace

SpoonerScenePreview3D::SpoonerScenePreview3D(const std::string& filePath)
{
    pugi::xml_document document;
    if (document.load_file(filePath.c_str()).status != pugi::status_ok)
        return;

    const auto root = document.child("SpoonerPlacements");
    if (!root)
        return;

    for (auto node = root.child("Placement"); node && placements.size() < kMaximumPreviewPlacements; node = node.next_sibling("Placement"))
    {
        const auto type = static_cast<EntityType>(node.child("Type").text().as_int());
        const GTAmodel::Model model(node.child("ModelHash").text().as_uint());
        if (type != EntityType::PED && type != EntityType::VEHICLE && type != EntityType::PROP)
            continue;
        if (model.hash == 0 || !model.IsValid() || !model.IsInCdImage())
            continue;
        if (type == EntityType::PED && !model.IsPed())
            continue;
        if (type == EntityType::VEHICLE && !model.IsVehicle())
            continue;
        if (type == EntityType::PROP && (model.IsPed() || model.IsVehicle()))
            continue;

        const auto positionRotation = node.child("PositionRotation");
        Placement placement;
        placement.type = type;
        placement.position = Vector3(positionRotation.child("X").text().as_float(), positionRotation.child("Y").text().as_float(), positionRotation.child("Z").text().as_float());
        placement.headingDegrees = positionRotation.child("Yaw").text().as_float();
        placement.preview = CreatePlacementPreview(type, model);
        if (type == EntityType::PED && placement.preview)
        {
            std::ostringstream pedPropertiesXml;
            node.child("PedProperties").print(pedPropertiesXml);
            placement.pedPropertiesXml = pedPropertiesXml.str();
            auto* pedPreview = static_cast<PedPreview3D*>(placement.preview.get());
            pedPreview->SetAnimations(ReadPedAnimations(node));
        }
        if (placement.preview)
            placements.push_back(std::move(placement));
    }

    if (placements.empty())
        return;

    Vector3 minimum = placements.front().position;
    Vector3 maximum = placements.front().position;
    for (const auto& placement : placements)
    {
        minimum.x = (std::min)(minimum.x, placement.position.x);
        minimum.y = (std::min)(minimum.y, placement.position.y);
        minimum.z = (std::min)(minimum.z, placement.position.z);
        maximum.x = (std::max)(maximum.x, placement.position.x);
        maximum.y = (std::max)(maximum.y, placement.position.y);
        maximum.z = (std::max)(maximum.z, placement.position.z);
    }

    sceneCenter = Vector3((minimum.x + maximum.x) * 0.5f, (minimum.y + maximum.y) * 0.5f, (minimum.z + maximum.z) * 0.5f);
    const Vector3 dimensions = maximum - minimum;
    sceneRadius = std::sqrt(dimensions.x * dimensions.x + dimensions.y * dimensions.y + dimensions.z * dimensions.z) * 0.5f;
    if (sceneRadius > kMaximumSceneRadius)
        sceneScale = kMaximumSceneRadius / sceneRadius;

    rootIndex = 0;
}

SpoonerScenePreview3D::~SpoonerScenePreview3D() = default;

void SpoonerScenePreview3D::Tick()
{
    if (nextPlacementIndex < placements.size())
    {
        auto& nextPlacement = placements[nextPlacementIndex];
        nextPlacement.preview->Tick();
        if (nextPlacement.preview->GetEntityHandle() != 0)
            nextPlacementIndex++;
    }

    for (std::size_t index = 0; index < placements.size(); index++)
    {
        if (placements[index].preview->GetEntityHandle() != 0)
        {
            if (placements[index].type == EntityType::PED && !placements[index].pedAppearanceApplied)
            {
                ApplyPedAppearance(GTAped(placements[index].preview->GetEntityHandle()), placements[index].pedPropertiesXml, placements[index].preview->GetFramingModel());
                placements[index].pedAppearanceApplied = true;
            }
            placements[index].preview->Tick();
        }

        if (placements[index].preview->GetEntityHandle() != 0)
        {
            rootIndex = index;
            return;
        }
    }
}

EntityType SpoonerScenePreview3D::GetRootType() const
{
    return placements.empty() ? EntityType::ALL : placements[rootIndex].type;
}

void* SpoonerScenePreview3D::GetRootAddress() const
{
    if (placements.empty())
        return nullptr;
    return placements[rootIndex].preview->GetRootAddress();
}

const GTAmodel::Model& SpoonerScenePreview3D::GetFramingModel() const
{
    return placements[rootIndex].preview->GetFramingModel();
}

float SpoonerScenePreview3D::GetSceneRadius() const
{
    float largestModelRadius = 0.0f;
    for (const auto& placement : placements)
        largestModelRadius = (std::max)(largestModelRadius, placement.preview->GetSceneRadius());
    return sceneRadius * sceneScale + largestModelRadius;
}

void SpoonerScenePreview3D::PlaceAt(const Vector3& center, float headingDegrees)
{
    const float headingRadians = headingDegrees * 0.017453292f;
    const float cosine = std::cos(headingRadians);
    const float sine = std::sin(headingRadians);
    for (auto& placement : placements)
    {
        const float localX = (placement.position.x - sceneCenter.x) * sceneScale;
        const float localY = (placement.position.y - sceneCenter.y) * sceneScale;
        const float localZ = (placement.position.z - sceneCenter.z) * sceneScale;
        const Vector3 position(center.x + localX * cosine - localY * sine, center.y + localX * sine + localY * cosine, center.z + localZ);
        placement.preview->PlaceAt(position, placement.headingDegrees + headingDegrees);
    }
}

void SpoonerScenePreview3D::GetExtraEntities(std::vector<UI3DScene::ExtraEntity>& extraEntities) const
{
    for (std::size_t index = 0; index < placements.size(); index++)
    {
        if (index == rootIndex)
            continue;

        const void* entityAddress = placements[index].preview->GetRootAddress();
        if (!entityAddress)
            continue;
        extraEntities.push_back({const_cast<void*>(entityAddress), placements[index].type == EntityType::PED, 255});
    }
}

bool SpoonerScenePreview3D::IsValid() const
{
    return !placements.empty();
}
} // namespace Previews
