/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#include "..\..\Menu\Language.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Menu\Routine.h"
#include "..\..\Menu\submenu_enum.h"
#include "..\..\Menu\submenu_switch.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\enums.h"
#include "..\..\Natives\natives2.h"
#include "..\..\macros.h"

#include <algorithm>
#include <string>
#include <vector>

namespace sub
{
namespace PedDamageTextures
{
namespace
{
int selectedBoneId = 0;

void ClearBloodDamageFromActivePed()
{
    GTAped activePed = g_activePedHandle;
    activePed.ClearBloodDamage();
}

void ClearVisibleDamageFromActivePed()
{
    GTAped activePed = g_activePedHandle;
    activePed.ResetVisibleDamage();
}

void DrawCategoryListMenu()
{
    AddTitle(Language::Translate("menu.player.wardrobe.damage.title"));
    AddOption(Language::Translate("menu.player.wardrobe.damage.option.packs.label"), null, nullFunc, SUB::PED_DAMAGE_PACKS);
    AddBreak(Language::Translate("menu.player.wardrobe.damage.section.wash_up.label"));
    AddTickol(Language::Translate("menu.player.wardrobe.damage.option.clear_blood.label"), true, ClearBloodDamageFromActivePed, ClearBloodDamageFromActivePed, TICKOL::CROSS);
    AddTickol(Language::Translate("menu.player.wardrobe.damage.option.clear_visible.label"), true, ClearVisibleDamageFromActivePed, ClearVisibleDamageFromActivePed, TICKOL::CROSS);
}

void DrawBoneSelectionMenu()
{
    AddTitle(Language::Translate("menu.player.wardrobe.damage.select_bone.title"));

    for (const auto& bone : Bone::vBoneNames)
    {
        bool boneSelected = false;
        AddTickol(bone.name, selectedBoneId == bone.boneid, boneSelected, boneSelected, TICKOL::SKULL_DM, TICKOL::NONE, true);
        if (boneSelected)
            selectedBoneId = bone.boneid;
    }
}

void DrawBloodMenu()
{
    GTAped activePed = g_activePedHandle;
    AddTitle(Language::Translate("menu.player.wardrobe.damage.blood.title"));

    for (const auto& bloodDecalName : PedBloodDecals::vBloodDecals)
    {
        bool bloodDecalSelected = false;
        AddOption(bloodDecalName, bloodDecalSelected);
        if (bloodDecalSelected)
            activePed.ApplyBlood(bloodDecalName, selectedBoneId);
    }
}

void DrawDamageDecalsMenu()
{
    GTAped activePed = g_activePedHandle;
    AddTitle(Language::Translate("menu.player.wardrobe.damage.decals.title"));

    for (const auto& damageDecalName : PedDamageDecals::vDamageDecals)
    {
        bool damageDecalSelected = false;
        AddOption(damageDecalName, damageDecalSelected);
        if (damageDecalSelected)
            activePed.ApplyDamageDecal(damageDecalName, selectedBoneId, Vector3(), 1.0f, 1.0f, 1, true);
    }
}

void DrawDamagePacksMenu()
{
    GTAped activePed = g_activePedHandle;
    const auto& damagePacks = PedDamagePacks::vDamagePacks;
    activePed.EnsureDamagePackTracking();

    AddTitle(Language::Translate("menu.player.wardrobe.damage.packs.title"));

    for (const auto& damagePackName : damagePacks)
    {
        bool applyDamagePackPressed = false;
        bool removeDamagePackPressed = false;
        const bool damagePackApplied = activePed.HasTrackedDamagePack(damagePackName);
        AddTickol(damagePackName, damagePackApplied, applyDamagePackPressed, removeDamagePackPressed, TICKOL::MAKEUPTHING);

        if (applyDamagePackPressed)
            activePed.ApplyDamagePack(damagePackName, 1.0f, 1.0f);

        if (removeDamagePackPressed)
            activePed.RemoveDamagePack(damagePackName);
    }
}
} // namespace
} // namespace PedDamageTextures
} // namespace sub

REGISTER_SUBMENU(PED_DAMAGE_CATEGORY_LIST, sub::PedDamageTextures::DrawCategoryListMenu)
REGISTER_SUBMENU(PED_DAMAGE_BONE_SELECTION, sub::PedDamageTextures::DrawBoneSelectionMenu)
REGISTER_SUBMENU(PED_DAMAGE_BLOOD, sub::PedDamageTextures::DrawBloodMenu)
REGISTER_SUBMENU(PED_DAMAGE_DECALS, sub::PedDamageTextures::DrawDamageDecalsMenu)
REGISTER_SUBMENU(PED_DAMAGE_PACKS, sub::PedDamageTextures::DrawDamagePacksMenu)
