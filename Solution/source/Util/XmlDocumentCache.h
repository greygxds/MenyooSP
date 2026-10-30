#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <pugixml/src/pugixml.hpp>

namespace Util
{
class XmlDocumentCache
{
  public:
    std::shared_ptr<pugi::xml_document> Load(const std::string& filePath)
    {
        if (filePath != cachedFilePath)
        {
            cachedFilePath = filePath;
            Invalidate();
        }

        const auto now = std::chrono::steady_clock::now();
        if (needsReload || now >= nextFileCheck)
        {
            std::error_code error;
            const auto lastWriteTime = std::filesystem::last_write_time(filePath, error);
            nextFileCheck = now + fileCheckInterval;

            if (needsReload || lastWriteTime != cachedWriteTime)
            {
                auto loadedDocument = std::make_shared<pugi::xml_document>();
                if (!loadedDocument->load_file(filePath.c_str()))
                    loadedDocument->reset();

                // Keep existing menu nodes alive until their frame finishes.
                document = std::move(loadedDocument);
                cachedWriteTime = lastWriteTime;
                needsReload = false;
            }
        }
        return document;
    }

    void Invalidate()
    {
        needsReload = true;
    }

  private:
    static constexpr std::chrono::seconds fileCheckInterval{1};
    std::string cachedFilePath;
    std::shared_ptr<pugi::xml_document> document;
    std::filesystem::file_time_type cachedWriteTime{};
    std::chrono::steady_clock::time_point nextFileCheck{};
    bool needsReload = true;
};
} // namespace Util
