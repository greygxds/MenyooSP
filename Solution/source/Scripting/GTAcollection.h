#pragma once

#include <string>
#include <vector>

#include "GTAped.h"

class GTAcollection final
{
  public:
    struct DrawableCollection
    {
        std::string name;
        int maxLocalId = -1;
        std::vector<int> localToGlobal;

        int GetGlobalDrawableIndex(int localId) const;
    };

    struct DrawableLocation
    {
        int collectionIndex = -1;
        int localId = -1;

        bool IsValid() const;
    };

    using DrawableCollections = std::vector<DrawableCollection>;

    static DrawableCollections GetComponentCollections(const GTAped& ped, int componentIndex);
    static DrawableCollections GetPropCollections(const GTAped& ped, int propIndex);
    static DrawableLocation FindDrawableLocation(const DrawableCollections& collections, int globalDrawableIndex);
    static int FindGlobalDrawableIndex(const DrawableCollections& collections, const std::string& collectionName, int localId);
    static std::string GetComponentCollectionString(const GTAped& ped, int componentIndex);
    static std::string GetPropCollectionString(const GTAped& ped, int propIndex);
};
