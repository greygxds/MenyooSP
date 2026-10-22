#pragma once

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace SaveFileSorting
{
enum class SortMode
{
    Name = 0,
    Created = 1,
    Modified = 2
};

inline std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

inline void SortDirectoryEntries(std::vector<std::string>& fileNames, const std::string& directoryPath, SortMode sortMode)
{
    struct DirectoryEntry
    {
        std::string name;
        bool isDirectory = false;
        FILETIME creationTime = {};
        FILETIME lastWriteTime = {};
    };

    std::vector<DirectoryEntry> entries;
    entries.reserve(fileNames.size());
    for (const auto& fileName : fileNames)
    {
        DirectoryEntry entry;
        entry.name = fileName;

        WIN32_FILE_ATTRIBUTE_DATA fileData{};
        if (GetFileAttributesExA((directoryPath + "\\" + fileName).c_str(), GetFileExInfoStandard, &fileData))
        {
            entry.isDirectory = (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            entry.creationTime = fileData.ftCreationTime;
            entry.lastWriteTime = fileData.ftLastWriteTime;
        }

        entries.push_back(std::move(entry));
    }

    std::sort(
        entries.begin(),
        entries.end(),
        [sortMode](const DirectoryEntry& left, const DirectoryEntry& right)
        {
            if (left.isDirectory != right.isDirectory)
                return left.isDirectory;

            const std::string leftLowerName = ToLower(left.name);
            const std::string rightLowerName = ToLower(right.name);

            if (sortMode != SortMode::Name)
            {
                const FILETIME& leftTime = sortMode == SortMode::Created ? left.creationTime : left.lastWriteTime;
                const FILETIME& rightTime = sortMode == SortMode::Created ? right.creationTime : right.lastWriteTime;
                const LONG timeComparison = CompareFileTime(&leftTime, &rightTime);
                if (timeComparison != 0)
                    return timeComparison > 0;
            }

            if (leftLowerName != rightLowerName)
                return leftLowerName < rightLowerName;
            return left.name < right.name;
        }
    );

    fileNames.clear();
    fileNames.reserve(entries.size());
    for (const auto& entry : entries)
        fileNames.push_back(entry.name);
}
} // namespace SaveFileSorting
