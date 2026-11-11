#include "PedOutfits.h"
#include "PedDecals.h"
#include "WardrobeCamera.h"
#include "..\..\Menu\Language.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Menu\Routine.h"
#include "..\..\Menu\submenu_enum.h"
#include "..\..\Menu\submenu_switch.h"
#include "..\..\Memory\GTAmemory.h"
#include "..\..\Natives\natives2.h"
#include "..\..\Scripting\GTAcollection.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\Game.h"
#include "..\..\macros.h"

#include <array>
#include <string>
#include <vector>

namespace sub
{ // Component changer

static int s_selectedComponentIndex = 0;
static int s_selectedPropIndex = 0;

static void AddPedComponentOption(const std::string& text, int componentSlotIndex)
{
    bool componentOptionPressed = false;
    AddOption(text, componentOptionPressed, nullFunc, SUB::PED_COMPONENT_VARIATIONS, true, true);
    if (componentOptionPressed)
    {
        s_selectedComponentIndex = componentSlotIndex;
    }
}

static void AddPedPropOption(const std::string& text, int propSlotIndex)
{
    bool propOptionPressed = false;
    AddOption(text, propOptionPressed, nullFunc, SUB::PED_PROP_VARIATIONS, true, true);
    if (propOptionPressed)
    {
        s_selectedPropIndex = propSlotIndex;
    }
}

namespace
{
enum class PedVariationKind
{
    Component,
    Prop
};

struct CollectionCacheEntry
{
    Hash cachedModelHash = 0;
    Ped cachedPedHandle = 0;
    GTAcollection::DrawableCollections collections;
    GTAcollection::DrawableLocation selectedDrawableLocation;
};

std::array<CollectionCacheEntry, PV_COMP_MAX> componentCollectionCache;
std::array<CollectionCacheEntry, PV_PROP_MAX> propCollectionCache;

struct CollectionMenuContext
{
    PedVariationKind variationKind = PedVariationKind::Component;
    int variationSlotIndex = 0;
    Ped pedHandle = 0;
};

CollectionMenuContext collectionMenuContext;

int GetCurrentDrawableIndex(const GTAped& ped, PedVariationKind variationKind, int variationSlotIndex)
{
    if (variationKind == PedVariationKind::Component)
        return ped.GetComponentDrawableIndex(variationSlotIndex);

    return ped.GetPropDrawableIndex(variationSlotIndex);
}

CollectionCacheEntry& GetCollectionCacheEntry(Ped pedHandle, PedVariationKind variationKind, int variationSlotIndex)
{
    CollectionCacheEntry& cache = variationKind == PedVariationKind::Component ? componentCollectionCache[variationSlotIndex] : propCollectionCache[variationSlotIndex];
    const Hash currentModelHash = GET_ENTITY_MODEL(pedHandle);

    if (cache.cachedModelHash != currentModelHash || cache.cachedPedHandle != pedHandle)
    {
        GTAped ped = pedHandle;
        if (variationKind == PedVariationKind::Component)
        {
            cache.collections = GTAcollection::GetComponentCollections(ped, variationSlotIndex);
        }
        else
        {
            cache.collections = GTAcollection::GetPropCollections(ped, variationSlotIndex);
        }
        cache.selectedDrawableLocation = GTAcollection::FindDrawableLocation(cache.collections, GetCurrentDrawableIndex(ped, variationKind, variationSlotIndex));
        cache.cachedModelHash = currentModelHash;
        cache.cachedPedHandle = pedHandle;
    }

    return cache;
}

void SyncCollectionSelectionToDrawableIndex(CollectionCacheEntry& cache, int drawableIndex)
{
    const GTAcollection::DrawableLocation location = GTAcollection::FindDrawableLocation(cache.collections, drawableIndex);
    if (location.IsValid())
        cache.selectedDrawableLocation = location;
}

void RestoreCollectionSelection(Ped pedHandle, PedVariationKind variationKind, int variationSlotIndex, int drawableIndex)
{
    CollectionCacheEntry& cache = GetCollectionCacheEntry(pedHandle, variationKind, variationSlotIndex);
    SyncCollectionSelectionToDrawableIndex(cache, drawableIndex);
}

struct ComponentVariationSelection
{
    int drawableIndex;
    int textureIndex;
};

bool FindValidComponentVariation(const GTAped& ped, int componentSlotIndex, int requestedDrawableIndex, int searchStep, int requestedTextureIndex, ComponentVariationSelection& selection)
{
    const int maxDrawableIndex = ped.GetComponentDrawableCount(componentSlotIndex) - 1;
    if (maxDrawableIndex < 0)
        return false;

    int candidateDrawableIndex = requestedDrawableIndex;
    int candidateTextureIndex = requestedTextureIndex;
    for (int attempt = 0; attempt <= maxDrawableIndex; ++attempt)
    {
        if (ped.IsComponentVariationValid(componentSlotIndex, candidateDrawableIndex, candidateTextureIndex))
        {
            selection = {candidateDrawableIndex, candidateTextureIndex};
            return true;
        }

        if (candidateTextureIndex != 0 && ped.IsComponentVariationValid(componentSlotIndex, candidateDrawableIndex, 0))
        {
            selection = {candidateDrawableIndex, 0};
            return true;
        }

        candidateDrawableIndex += searchStep;
        if (candidateDrawableIndex > maxDrawableIndex)
            candidateDrawableIndex = 0;
        if (candidateDrawableIndex < 0)
            candidateDrawableIndex = maxDrawableIndex;
        candidateTextureIndex = 0;
    }

    return false;
}

void OpenCollectionList(PedVariationKind variationKind, int variationSlotIndex, Ped pedHandle)
{
    collectionMenuContext = {variationKind, variationSlotIndex, pedHandle};
    Menu::pendingSubmenu = SUB::PED_VARIATION_COLLECTIONS;
}

bool DrawCollectionControls(CollectionCacheEntry& cache, PedVariationKind variationKind, int variationSlotIndex, Ped pedHandle, int previousDrawableIndex, int& drawableIndex)
{
    if (cache.collections.empty())
        return false;

    AddBreak(Language::Translate("menu.player.wardrobe.variation.section.collection_data.label"));

    if (cache.selectedDrawableLocation.collectionIndex < 0 || cache.selectedDrawableLocation.collectionIndex >= static_cast<int>(cache.collections.size()))
        cache.selectedDrawableLocation = {0, 0};

    std::vector<std::string> collectionNames;
    for (const auto& collection : cache.collections)
        collectionNames.push_back(collection.name);

    const int previousCollectionIndex = cache.selectedDrawableLocation.collectionIndex;
    bool openCollectionList = false;
    cache.selectedDrawableLocation.collectionIndex =
        AddTexterCycler(Language::Translate("menu.player.wardrobe.variation.option.collection.label"), cache.selectedDrawableLocation.collectionIndex, collectionNames, openCollectionList);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.variation.option.collection.description"));
    if (openCollectionList)
        OpenCollectionList(variationKind, variationSlotIndex, pedHandle);

    const bool collectionChanged = cache.selectedDrawableLocation.collectionIndex != previousCollectionIndex;
    if (collectionChanged)
        cache.selectedDrawableLocation.localId = 0;

    auto& collection = cache.collections[cache.selectedDrawableLocation.collectionIndex];
    if (collection.localToGlobal.empty() || collection.maxLocalId < 0)
        return false;

    if (cache.selectedDrawableLocation.localId < 0 || cache.selectedDrawableLocation.localId > collection.maxLocalId)
        cache.selectedDrawableLocation.localId = 0;

    const int previousLocalId = cache.selectedDrawableLocation.localId;
    AddNumberStepper(Language::Translate("menu.player.wardrobe.variation.option.local_id.label"), cache.selectedDrawableLocation.localId, 0, 1.0, 0, collection.maxLocalId, false, true);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.variation.option.index.description"));

    if (collectionChanged || cache.selectedDrawableLocation.localId != previousLocalId)
    {
        drawableIndex = collection.GetGlobalDrawableIndex(cache.selectedDrawableLocation.localId);
        return true;
    }

    if (drawableIndex != previousDrawableIndex)
        SyncCollectionSelectionToDrawableIndex(cache, drawableIndex);

    return false;
}
} // namespace

namespace
{
bool ConfirmOnSecondPress(bool& confirmationPending, const std::string& localizedMessage)
{
    if (!confirmationPending)
    {
        Game::Print::ShowNotification(localizedMessage);
        confirmationPending = true;
        return false;
    }
    confirmationPending = false;
    return true;
}
} // namespace

void PedComponentsMenu()
{
    bool toggleFrontViewPressed = false;
    bool randomizeComponentsPressed = false;
    bool restoreDefaultComponentsPressed = false;
    bool restoreDefaultComponentsAndClearPropsPressed = false;
    static bool randomComponentsConfirmationPending = false;
    static bool defaultComponentsConfirmationPending = false;
    static bool defaultComponentsAndAccessoriesConfirmationPending = false;

    PedOutfitMenu::ResetBrowserState();

    GTAped ped = g_activePedHandle;

    WardrobeCamera::SetFraming(ped, WardrobeCamera::Framing::Body);

    AddTitle(Language::Translate("menu.player.wardrobe.wardrobe.title"));
    AddLocal(Language::Translate("menu.player.wardrobe.wardrobe.option.front_view.label"), WardrobeCamera::IsActive(), toggleFrontViewPressed, toggleFrontViewPressed);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.wardrobe.option.front_view.description"));
    AddOption(Language::Translate("menu.player.wardrobe.wardrobe.option.outfits.label"), null, nullFunc, SUB::PED_OUTFITS);
    AddOption(Language::Translate("menu.player.wardrobe.wardrobe.option.default_outfits.label"), null, nullFunc, SUB::PED_DEFAULT_OUTFITS);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.wardrobe.option.default_outfits.description"));
    AddOption(Language::Translate("menu.player.wardrobe.wardrobe.option.decals.label"), null, PedDecals::OpenDecalsMenu, -1, true);
    AddOption(Language::Translate("menu.player.wardrobe.wardrobe.option.damage.label"), null, nullFunc, SUB::PED_DAMAGE_CATEGORY_LIST);
    AddOption(Language::Translate("menu.player.wardrobe.wardrobe.option.head_features.label"), null, nullFunc, SUB::PED_HEAD);
    AddOption(Language::Translate("menu.player.wardrobe.wardrobe.option.accessories.label"), null, nullFunc, SUB::PED_PROPS);

    const std::vector<std::string> componentSlotNames{
        "Head ~c~[head]",
        "Beard/Mask ~c~[berd]",
        "Hair ~c~[hair]",
        "Torso ~c~[uppr]",
        "Legs ~c~[lowr]",
        "Hands/Back ~c~[hand]",
        "Shoes ~c~[feet]",
        "Teeth/Scarf/Necklace/Bracelets ~c~[teef]",
        "Accessory/Tops ~c~[accs]",
        "Task/Armour ~c~[task]",
        "Emblem ~c~[decl]",
        "Tops2 (Outer) ~c~[jbib]"
    };

    AddBreak(Language::Translate("menu.player.wardrobe.sections.section.components.label"));

    for (int componentSlotIndex = 0; componentSlotIndex < PV_COMP_MAX; ++componentSlotIndex)
    {
        if (ped.GetComponentDrawableCount(componentSlotIndex) > 0)
            AddPedComponentOption(componentSlotNames[componentSlotIndex], componentSlotIndex);
    }

    AddBreak(Language::Translate("menu.player.wardrobe.sections.section.utilities.label"));
    AddOption(Language::Translate("menu.player.wardrobe.option.random.label"), randomizeComponentsPressed);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.option.random.description"));
    AddOption(Language::Translate("menu.player.wardrobe.option.default.label"), restoreDefaultComponentsPressed);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.option.default.description"));
    AddOption(Language::Translate("menu.player.wardrobe.option.default_all.label"), restoreDefaultComponentsAndClearPropsPressed);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.option.default_all.description"));

    if (randomizeComponentsPressed)
    {
        if (ConfirmOnSecondPress(randomComponentsConfirmationPending, Language::Translate("menu.player.wardrobe.confirm.option.random_components.label")))
        {
            ped.RequestControlOnce();
            ped.SetRandomComponentVariation();
        }
        return;
    }

    if (restoreDefaultComponentsPressed)
    {
        if (ConfirmOnSecondPress(defaultComponentsConfirmationPending, Language::Translate("menu.player.wardrobe.confirm.option.default_components.label")))
        {
            ped.RequestControlOnce();
            ped.SetDefaultComponentVariation();
        }
        return;
    }

    if (restoreDefaultComponentsAndClearPropsPressed)
    {
        if (ConfirmOnSecondPress(defaultComponentsAndAccessoriesConfirmationPending, Language::Translate("menu.player.wardrobe.confirm.option.default_components_and_accessories.label")))
        {
            ped.RequestControlOnce();
            ped.SetDefaultComponentVariation();
            ped.ClearAllProps();
        }
        return;
    }
    if (toggleFrontViewPressed)
    {
        if (WardrobeCamera::IsActive())
            WardrobeCamera::Disable();
        else
            WardrobeCamera::Enable(ped);
        return;
    }
}

void PedComponentVariationsMenu()
{
    GTAped ped = g_activePedHandle;
    int componentDrawableIndex = ped.GetComponentDrawableIndex(s_selectedComponentIndex);
    int componentTextureIndex = ped.GetComponentTextureIndex(s_selectedComponentIndex);
    const int previousComponentDrawableIndex = componentDrawableIndex;
    const int previousComponentTextureIndex = componentTextureIndex;

    const int maxComponentDrawableIndex = ped.GetComponentDrawableCount(s_selectedComponentIndex) - 1;
    int componentTextureCount = ped.GetComponentTextureCount(s_selectedComponentIndex, componentDrawableIndex);

    AddTitle(Language::Translate("menu.player.wardrobe.variation.title"));

    if (maxComponentDrawableIndex >= 0)
        AddNumberStepper(Language::Translate("menu.player.wardrobe.variation.option.type.label"), componentDrawableIndex, 0, 1.0, 0, maxComponentDrawableIndex, false, true);

    if (componentDrawableIndex != previousComponentDrawableIndex)
    {
        componentTextureIndex = 0;
        componentTextureCount = ped.GetComponentTextureCount(s_selectedComponentIndex, componentDrawableIndex);
    }

    if (componentTextureCount > 0)
        AddNumberStepper(Language::Translate("menu.player.wardrobe.variation.option.texture.label"), componentTextureIndex, 0, 1.0, 0, componentTextureCount - 1, false, true);

    bool collectionVariationChanged = false;
    if (!g_isEnhanced && maxComponentDrawableIndex >= 0)
    {
        auto& collectionCache = GetCollectionCacheEntry(g_activePedHandle, PedVariationKind::Component, s_selectedComponentIndex);
        collectionVariationChanged =
            DrawCollectionControls(collectionCache, PedVariationKind::Component, s_selectedComponentIndex, g_activePedHandle, previousComponentDrawableIndex, componentDrawableIndex);
        if (collectionVariationChanged)
            componentTextureIndex = 0;
    }

    if (collectionVariationChanged || componentDrawableIndex != previousComponentDrawableIndex || componentTextureIndex != previousComponentTextureIndex)
    {
        const int searchStep = componentDrawableIndex < previousComponentDrawableIndex ? -1 : 1;
        ComponentVariationSelection selection{};
        if (FindValidComponentVariation(ped, s_selectedComponentIndex, componentDrawableIndex, searchStep, componentTextureIndex, selection))
        {
            if (s_selectedComponentIndex == PV_COMP_ACCS && !GET_PED_CONFIG_FLAG(ped.GetHandle(), ePedConfigFlags::DisableTakeOffScubaGear, true))
                SET_PED_CONFIG_FLAG(ped.GetHandle(), ePedConfigFlags::DisableTakeOffScubaGear, true);
            ped.SetComponentVariation(s_selectedComponentIndex, selection.drawableIndex, selection.textureIndex);
            componentDrawableIndex = selection.drawableIndex;
            componentTextureIndex = selection.textureIndex;
        }
        else
        {
            componentDrawableIndex = previousComponentDrawableIndex;
            componentTextureIndex = previousComponentTextureIndex;
            if (collectionVariationChanged)
                RestoreCollectionSelection(g_activePedHandle, PedVariationKind::Component, s_selectedComponentIndex, previousComponentDrawableIndex);
        }
    }
}

void PedPropsMenu()
{
    bool randomizePropsPressed = false;
    bool clearPropsPressed = false;
    static bool randomPropsConfirmationPending = false;
    static bool clearPropsConfirmationPending = false;

    GTAped ped = g_activePedHandle;

    WardrobeCamera::SetFraming(ped, WardrobeCamera::Framing::Head);
    const std::array<const char*, PV_PROP_MAX> propSlotNames{
        "Hats ~c~[p_head]", "Glasses ~c~[p_eyes]", "Ear Pieces ~c~[p_ears]", "Unknown 3", "Unknown 4", "Unknown 5", "Watches ~c~[p_lwrist]", "Bangles ~c~[p_rwrist]", "Unknown 8", "Unknown 9"
    };

    AddTitle(Language::Translate("menu.player.wardrobe.accessories.title"));

    for (int propSlotIndex = 0; propSlotIndex < static_cast<int>(propSlotNames.size()); ++propSlotIndex)
    {
        if (ped.GetPropDrawableCount(propSlotIndex) > 0)
            AddPedPropOption(propSlotNames[propSlotIndex], propSlotIndex);
    }

    AddBreak(Language::Translate("menu.player.wardrobe.sections.section.utilities.label"));
    AddOption(Language::Translate("menu.player.wardrobe.accessories.option.random.label"), randomizePropsPressed);
    AddTickol(Language::Translate("menu.player.wardrobe.accessories.option.default.label"), true, clearPropsPressed, clearPropsPressed, TICKOL::CROSS);

    if (randomizePropsPressed)
    {
        if (ConfirmOnSecondPress(randomPropsConfirmationPending, Language::Translate("menu.player.wardrobe.confirm.option.random_accessories.label")))
        {
            ped.RequestControlOnce();
            ped.SetRandomProps();
        }
        return;
    }

    if (clearPropsPressed)
    {
        if (ConfirmOnSecondPress(clearPropsConfirmationPending, Language::Translate("menu.player.wardrobe.confirm.option.clear_accessories.label")))
        {
            ped.RequestControlOnce();
            ped.ClearAllProps();
        }
        return;
    }
}

void PedPropVariationsMenu()
{
    GTAped ped = g_activePedHandle;
    int propDrawableIndex = ped.GetPropDrawableIndex(s_selectedPropIndex);
    int propTextureIndex = ped.GetPropTextureIndex(s_selectedPropIndex);
    const int previousPropDrawableIndex = propDrawableIndex;
    const int previousPropTextureIndex = propTextureIndex;

    const int maxPropDrawableIndex = ped.GetPropDrawableCount(s_selectedPropIndex) - 1;
    int propTextureCount = propDrawableIndex >= 0 ? ped.GetPropTextureCount(s_selectedPropIndex, propDrawableIndex) : 0;

    AddTitle(Language::Translate("menu.player.wardrobe.variation.title"));

    if (maxPropDrawableIndex >= 0)
    {
        AddNumberStepper(Language::Translate("menu.player.wardrobe.variation.option.type.label"), propDrawableIndex, 0, 1.0, -1, maxPropDrawableIndex, false, true);
    }

    if (propDrawableIndex != previousPropDrawableIndex)
    {
        propTextureIndex = 0;
        propTextureCount = propDrawableIndex >= 0 ? ped.GetPropTextureCount(s_selectedPropIndex, propDrawableIndex) : 0;
    }
    if (propTextureCount > 0)
    {
        AddNumberStepper(Language::Translate("menu.player.wardrobe.variation.option.texture.label"), propTextureIndex, 0, 1.0, 0, propTextureCount - 1, false, true);
    }

    bool collectionVariationChanged = false;
    if (!g_isEnhanced && maxPropDrawableIndex >= 0)
    {
        auto& collectionCache = GetCollectionCacheEntry(g_activePedHandle, PedVariationKind::Prop, s_selectedPropIndex);
        collectionVariationChanged = DrawCollectionControls(collectionCache, PedVariationKind::Prop, s_selectedPropIndex, g_activePedHandle, previousPropDrawableIndex, propDrawableIndex);
        if (collectionVariationChanged)
            propTextureIndex = 0;
    }

    if (collectionVariationChanged || propDrawableIndex != previousPropDrawableIndex || propTextureIndex != previousPropTextureIndex)
    {
        if (ped.IsPropVariationValid(s_selectedPropIndex, propDrawableIndex, propTextureIndex))
        {
            ped.SetPropVariation(s_selectedPropIndex, propDrawableIndex, propTextureIndex, NETWORK_IS_GAME_IN_PROGRESS());
        }
        else
        {
            propDrawableIndex = previousPropDrawableIndex;
            propTextureIndex = previousPropTextureIndex;
            if (collectionVariationChanged)
                RestoreCollectionSelection(g_activePedHandle, PedVariationKind::Prop, s_selectedPropIndex, previousPropDrawableIndex);
        }
    }
}

void PedVariationCollectionsMenu()
{
    AddTitle(Language::Translate("menu.player.wardrobe.variation.collection.title"));

    const int variationSlotCount = collectionMenuContext.variationKind == PedVariationKind::Component ? PV_COMP_MAX : PV_PROP_MAX;
    if (collectionMenuContext.variationSlotIndex < 0 || collectionMenuContext.variationSlotIndex >= variationSlotCount || !DOES_ENTITY_EXIST(collectionMenuContext.pedHandle))
    {
        Menu::SetPreviousMenu();
        return;
    }

    CollectionCacheEntry& collectionCache = GetCollectionCacheEntry(collectionMenuContext.pedHandle, collectionMenuContext.variationKind, collectionMenuContext.variationSlotIndex);
    for (int collectionIndex = 0; collectionIndex < static_cast<int>(collectionCache.collections.size()); ++collectionIndex)
    {
        const auto& collection = collectionCache.collections[collectionIndex];
        if (collection.localToGlobal.empty())
            continue;

        bool collectionSelected = false;
        bool collectionDeselected = false;
        AddTickol(collection.name, collectionIndex == collectionCache.selectedDrawableLocation.collectionIndex, collectionSelected, collectionDeselected, TICKOL::TICK, TICKOL::NONE);
        if (!collectionSelected && !collectionDeselected)
            continue;

        GTAped ped = collectionMenuContext.pedHandle;
        const int previousDrawableIndex = GetCurrentDrawableIndex(ped, collectionMenuContext.variationKind, collectionMenuContext.variationSlotIndex);
        int textureIndex = 0;
        collectionCache.selectedDrawableLocation = {collectionIndex, 0};

        const int firstDrawableIndex = collection.GetGlobalDrawableIndex(0);
        if (collectionMenuContext.variationKind == PedVariationKind::Component)
        {
            const int searchStep = firstDrawableIndex < previousDrawableIndex ? -1 : 1;
            ComponentVariationSelection selection{};
            if (FindValidComponentVariation(ped, collectionMenuContext.variationSlotIndex, firstDrawableIndex, searchStep, textureIndex, selection))
            {
                if (collectionMenuContext.variationSlotIndex == PV_COMP_ACCS && !GET_PED_CONFIG_FLAG(ped.GetHandle(), ePedConfigFlags::DisableTakeOffScubaGear, true))
                    SET_PED_CONFIG_FLAG(ped.GetHandle(), ePedConfigFlags::DisableTakeOffScubaGear, true);
                ped.SetComponentVariation(collectionMenuContext.variationSlotIndex, selection.drawableIndex, selection.textureIndex);
                SyncCollectionSelectionToDrawableIndex(collectionCache, selection.drawableIndex);
            }
            else
            {
                SyncCollectionSelectionToDrawableIndex(collectionCache, previousDrawableIndex);
            }
        }
        else
        {
            if (ped.IsPropVariationValid(collectionMenuContext.variationSlotIndex, firstDrawableIndex, textureIndex))
            {
                ped.SetPropVariation(collectionMenuContext.variationSlotIndex, firstDrawableIndex, textureIndex, NETWORK_IS_GAME_IN_PROGRESS());
                SyncCollectionSelectionToDrawableIndex(collectionCache, firstDrawableIndex);
            }
            else
            {
                SyncCollectionSelectionToDrawableIndex(collectionCache, previousDrawableIndex);
            }
        }

        Menu::SetPreviousMenu();
        return;
    }
}

} // namespace sub

REGISTER_SUBMENU(PED_WARDROBE, sub::PedComponentsMenu)
REGISTER_SUBMENU(PED_COMPONENT_VARIATIONS, sub::PedComponentVariationsMenu)
REGISTER_SUBMENU(PED_PROPS, sub::PedPropsMenu)
REGISTER_SUBMENU(PED_PROP_VARIATIONS, sub::PedPropVariationsMenu)
REGISTER_SUBMENU(PED_VARIATION_COLLECTIONS, sub::PedVariationCollectionsMenu)
