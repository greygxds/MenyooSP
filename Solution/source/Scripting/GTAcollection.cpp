#include "GTAcollection.h"

#include "../Memory/GTAmemory.h"

#include <utility>

int GTAcollection::DrawableCollection::GetGlobalDrawableIndex(int localId) const
{
    if (localId < 0 || localId > maxLocalId || localId >= static_cast<int>(localToGlobal.size()))
        return -1;

    return localToGlobal[localId];
}

bool GTAcollection::DrawableLocation::IsValid() const
{
    return collectionIndex >= 0 && localId >= 0;
}

GTAcollection::DrawableCollections GTAcollection::GetComponentCollections(const GTAped& ped, int componentIndex)
{
    GTAmemory::DrawableCollectionData collectionData = GTAmemory::BuildDrawableCollectionData(ped.GetHandle(), componentIndex);
    DrawableCollections collections;
    collections.reserve(collectionData.collections.size());

    for (auto& collection : collectionData.collections)
        collections.push_back({std::move(collection.name), collection.maxLocalId, std::move(collection.localToGlobal)});

    return collections;
}

GTAcollection::DrawableCollections GTAcollection::GetPropCollections(const GTAped& ped, int propIndex)
{
    GTAmemory::DrawableCollectionData collectionData = GTAmemory::BuildPropCollectionData(ped.GetHandle(), propIndex);
    DrawableCollections collections;
    collections.reserve(collectionData.collections.size());

    for (auto& collection : collectionData.collections)
        collections.push_back({std::move(collection.name), collection.maxLocalId, std::move(collection.localToGlobal)});

    return collections;
}

GTAcollection::DrawableLocation GTAcollection::FindDrawableLocation(const DrawableCollections& collections, int globalDrawableIndex)
{
    for (int collectionIndex = 0; collectionIndex < static_cast<int>(collections.size()); ++collectionIndex)
    {
        const DrawableCollection& collection = collections[collectionIndex];
        for (int localId = 0; localId < static_cast<int>(collection.localToGlobal.size()); ++localId)
        {
            if (collection.localToGlobal[localId] == globalDrawableIndex)
                return {collectionIndex, localId};
        }
    }

    return {};
}

int GTAcollection::FindGlobalDrawableIndex(const DrawableCollections& collections, const std::string& collectionName, int localId)
{
    for (const DrawableCollection& collection : collections)
    {
        if (collection.name != collectionName)
            continue;

        return collection.GetGlobalDrawableIndex(localId);
    }

    return -1;
}

std::string GTAcollection::GetComponentCollectionString(const GTAped& ped, int componentIndex)
{
    return GTAmemory::GetPedDrawableCollectionString(ped.GetHandle(), componentIndex);
}

std::string GTAcollection::GetPropCollectionString(const GTAped& ped, int propIndex)
{
    return GTAmemory::GetPedPropCollectionString(ped.GetHandle(), propIndex);
}
