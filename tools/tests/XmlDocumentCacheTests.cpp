#include "../../Solution/source/Util/XmlDocumentCache.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void Write(const std::filesystem::path& path, const char* xml)
{
    std::ofstream stream(path);
    stream << xml;
    Require(stream.good(), "Unable to write test fixture");
}

void WaitForFileCheck()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}
} // namespace

int main()
{
    try
    {
        const auto directory = std::filesystem::temp_directory_path() / ("MenyooXmlCacheTests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        Require(std::filesystem::create_directory(directory), "Unable to create test directory");
        const auto file = directory / "favourites.xml";
        const auto otherFile = directory / "other.xml";
        Util::XmlDocumentCache cache;

        const auto missingDocument = cache.Load(file.string());
        Require(!missingDocument->document_element(), "Missing file must produce an empty document");
        Require(cache.Load(file.string()) == missingDocument, "Missing file must stay cached");

        Write(file, "<FavouritePeds><Ped hash='0x123' customName='Original'/></FavouritePeds>");
        WaitForFileCheck();
        const auto firstDocument = cache.Load(file.string());
        const auto originalPed = firstDocument->document_element().first_child();
        Require(std::string(originalPed.attribute("customName").value()) == "Original", "Newly created file was not detected");
        for (int frame = 0; frame < 10000; ++frame)
            Require(cache.Load(file.string()) == firstDocument, "Unchanged file was reloaded");
        WaitForFileCheck();
        Require(cache.Load(file.string()) == firstDocument, "Timestamp check must not reload unchanged XML");

        const auto originalWriteTime = std::filesystem::last_write_time(file);
        Write(file, "<FavouritePeds><Ped hash='0x456' customName='Updated'/></FavouritePeds>");
        std::filesystem::last_write_time(file, originalWriteTime);
        cache.Invalidate();
        const auto updatedDocument = cache.Load(file.string());
        Require(std::string(updatedDocument->document_element().first_child().attribute("hash").value()) == "0x456", "Invalidation must refresh immediately even with the same timestamp");
        Require(std::string(originalPed.attribute("customName").value()) == "Original", "Refresh invalidated a menu's existing nodes");

        Write(file, "<AddedVehicleModels><VehModel modelHash='0x789'><VehicleProperties><Livery>7</Livery></VehicleProperties></VehModel></AddedVehicleModels>");
        std::filesystem::last_write_time(file, originalWriteTime + std::chrono::seconds(2));
        WaitForFileCheck();
        const auto vehicleDocument = cache.Load(file.string());
        Require(vehicleDocument->child("AddedVehicleModels").first_child().child("VehicleProperties").child("Livery").text().as_int() == 7, "External edits or saved vehicle properties were lost");

        std::filesystem::remove(file);
        WaitForFileCheck();
        const auto deletedDocument = cache.Load(file.string());
        Require(!deletedDocument->document_element(), "Deleted file must clear favourites");
        Require(cache.Load(file.string()) == deletedDocument, "Deleted file must stay cached");

        Write(file, "<FavouritePeds><Ped");
        cache.Invalidate();
        const auto invalidDocument = cache.Load(file.string());
        Require(!invalidDocument->document_element(), "Malformed XML must not expose partially parsed entries");
        Require(cache.Load(file.string()) == invalidDocument, "Malformed XML must stay cached");

        Write(otherFile, "<FavouritePeds><Ped customName='Other file'/></FavouritePeds>");
        Require(std::string(cache.Load(otherFile.string())->document_element().first_child().attribute("customName").value()) == "Other file", "Changing file paths must refresh immediately");

        std::filesystem::remove(file);
        std::filesystem::remove(otherFile);
        std::filesystem::remove(directory);
        std::cout << "XML cache regression checks passed\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
