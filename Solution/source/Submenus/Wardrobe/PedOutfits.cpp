#include "PedOutfits.h"
#include "PedOutfitPersistence.h"
#include "..\..\macros.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Menu\Keybinds.h"
#include "..\..\Menu\Routine.h"
#include "..\..\Menu\StatsPanel.h"
#include "..\..\Menu\Language.h"
#include "..\..\Natives\natives2.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Scripting\ModelNames.h"
#include "..\..\Scripting\Game.h"
#include "..\..\Util\ExePath.h"
#include "..\..\Util\FileLogger.h"
#include "..\..\Util\StringManip.h"
#include "..\..\Util\keyboard.h"
#include "..\..\Menu\FolderPreviewBmps.h"
#include "..\Previews\Preview3DManager.h"

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <cstdio>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <cstdint>
#include <pugixml\src\pugixml.hpp>
#include <dirent\include\dirent.h>

namespace sub
{
namespace
{
struct OutfitMenuState
{
    std::string searchText;
    std::string directoryPath;
    std::string selectedOutfitPath;
    std::string selectedOutfitName;
    std::uint8_t attachmentPersistenceModeIndex = 0;
    bool useLegacyXmlFormat = false;
};

OutfitMenuState outfitMenuState;

PedOutfitPersistence::OutfitXmlFormat GetSelectedXmlFormat()
{
    if (outfitMenuState.useLegacyXmlFormat)
        return PedOutfitPersistence::OutfitXmlFormat::Legacy;
    return PedOutfitPersistence::OutfitXmlFormat::Readable;
}

PedOutfitPersistence::AttachmentPersistenceMode GetAttachmentPersistenceMode()
{
    switch (outfitMenuState.attachmentPersistenceModeIndex)
    {
    case 1:
        return PedOutfitPersistence::AttachmentPersistenceMode::ForceOff;
    case 2:
        return PedOutfitPersistence::AttachmentPersistenceMode::ForceOn;
    default:
        return PedOutfitPersistence::AttachmentPersistenceMode::UseFileSetting;
    }
}
} // namespace

namespace
{
struct OutfitFileStats
{
    Hash modelHash = 0;
    std::string modelLabel = "---";
    int opacityLevel = 255;
    bool isVisible = true;
    std::string facialMood;
    int decalCount = 0;
    int attachmentCount = 0;
    std::vector<std::pair<std::string, std::string>> componentValues;
    std::vector<std::pair<std::string, std::string>> propValues;
};

OutfitFileStats ReadOutfitFileStats(const std::string& filePath)
{
    constexpr const char* componentSlotNames[] = {"head", "berd", "hair", "uppr", "lowr", "hand", "feet", "teef", "accs", "task", "decl", "jbib"};
    constexpr const char* propSlotNames[] = {"p_head", "p_eyes", "p_ears", "p_mouth", "p_lhand", "p_rhand", "p_lwrist", "p_rwrist", "p_hip", "p_lfoot", "p_rfoot", "p_unk1", "p_unk2"};
    constexpr int displayedPropSlots[] = {0, 1, 2, 6, 7};

    OutfitFileStats stats;
    pugi::xml_document document;
    if (document.load_file(filePath.c_str()).status != pugi::status_ok)
        return stats;

    const auto outfitRoot = document.child("OutfitPedData");
    if (!outfitRoot)
        return stats;

    const Model pedModel = outfitRoot.child("ModelHash").text().as_uint();
    stats.modelHash = pedModel.hash;
    stats.modelLabel = GetPedModelLabel(pedModel, true);
    if (stats.modelLabel.empty())
        stats.modelLabel = IntToHexString(pedModel.hash, true);

    stats.opacityLevel = outfitRoot.child("OpacityLevel").text().as_int(255);
    const auto visibleNode = outfitRoot.child("IsVisible");
    if (visibleNode)
        stats.isVisible = visibleNode.text().as_bool();

    const auto pedProperties = outfitRoot.child("PedProperties");
    stats.facialMood = pedProperties.child("FacialMood").text().as_string();
    for (auto decalNode = pedProperties.child("TattooLogoDecals").first_child(); decalNode; decalNode = decalNode.next_sibling())
        stats.decalCount++;

    int componentSlotIndex = 0;
    for (auto componentNode = pedProperties.child("PedComps").first_child(); componentNode; componentNode = componentNode.next_sibling())
    {
        std::string slotLabel = componentNode.name();
        if (slotLabel.empty() || slotLabel[0] == '_')
        {
            slotLabel = "Comp " + std::to_string(componentSlotIndex);
            if (componentSlotIndex < std::size(componentSlotNames))
                slotLabel = componentSlotNames[componentSlotIndex];
        }
        stats.componentValues.push_back({slotLabel, componentNode.text().as_string()});
        componentSlotIndex++;
    }

    int propSlotIndex = 0;
    for (auto propNode = pedProperties.child("PedProps").first_child(); propNode; propNode = propNode.next_sibling())
    {
        bool displayPropSlot = false;
        for (const int displayedPropSlot : displayedPropSlots)
        {
            if (displayedPropSlot == propSlotIndex)
            {
                displayPropSlot = true;
                break;
            }
        }

        if (displayPropSlot)
        {
            std::string slotLabel = propNode.name();
            if (slotLabel.empty() || slotLabel[0] == '_')
            {
                slotLabel = "Prop " + std::to_string(propSlotIndex);
                if (propSlotIndex < std::size(propSlotNames))
                    slotLabel = propSlotNames[propSlotIndex];
            }
            stats.propValues.push_back({slotLabel, propNode.text().as_string()});
        }
        propSlotIndex++;
    }

    const auto attachmentsNode = outfitRoot.child("SpoonerAttachments");
    for (auto attachmentNode = attachmentsNode.child("Attachment"); attachmentNode; attachmentNode = attachmentNode.next_sibling("Attachment"))
        stats.attachmentCount++;

    return stats;
}

std::vector<StatsPanel::Row> BuildOutfitFileStatsRows(const std::string& filePath, const OutfitFileStats& stats)
{
    std::vector<StatsPanel::Row> rows;
    rows.push_back(StatsPanel::StatRow{Language::Translate("component.wardrobe_stats.model.label"), stats.modelLabel});
    if (stats.opacityLevel < 255)
        rows.push_back(StatsPanel::StatRow{Language::Translate("component.wardrobe_stats.opacity.label"), std::to_string(stats.opacityLevel)});
    if (!stats.isVisible)
        rows.push_back(StatsPanel::StatRow{Language::Translate("component.wardrobe_stats.visible.label"), Language::Translate("menu.stats_panel.option.no.label")});
    if (!stats.facialMood.empty())
        rows.push_back(StatsPanel::StatRow{Language::Translate("component.wardrobe_stats.mood.label"), stats.facialMood});
    if (stats.decalCount > 0)
        rows.push_back(StatsPanel::StatRow{Language::Format("component.wardrobe_stats.decals.label", {{"count", static_cast<std::uint64_t>(stats.decalCount)}}), std::to_string(stats.decalCount)});

    const size_t slotRowCount = (std::max)(stats.componentValues.size(), stats.propValues.size());
    if (slotRowCount > 0)
    {
        rows.push_back(StatsPanel::SeparatorRow{});
        rows.push_back(StatsPanel::DoubleStatRow{Language::Translate("component.wardrobe_stats.components.label"), "", Language::Translate("component.wardrobe_stats.props.label"), ""});
        for (size_t slotIndex = 0; slotIndex < slotRowCount; slotIndex++)
        {
            std::string componentLabel;
            std::string componentValue;
            if (slotIndex < stats.componentValues.size())
            {
                componentLabel = stats.componentValues[slotIndex].first;
                componentValue = stats.componentValues[slotIndex].second;
            }

            std::string propLabel;
            std::string propValue;
            if (slotIndex < stats.propValues.size())
            {
                propLabel = stats.propValues[slotIndex].first;
                propValue = stats.propValues[slotIndex].second;
            }

            rows.push_back(StatsPanel::DoubleStatRow{componentLabel, componentValue, propLabel, propValue});
        }
        rows.push_back(StatsPanel::SeparatorRow{});
    }

    rows.push_back(
        StatsPanel::StatRow{Language::Format("component.wardrobe_stats.attachments.label", {{"count", static_cast<std::uint64_t>(stats.attachmentCount)}}), std::to_string(stats.attachmentCount)}
    );
    rows.push_back(StatsPanel::StatRow{Language::Translate("menu.stats_panel.option.size.label"), GetFileSizeStr(filePath)});
    rows.push_back(StatsPanel::StatRow{Language::Translate("menu.stats_panel.option.modified.label"), GetFileLastWriteDateStr(filePath)});
    return rows;
}

void DrawOutfitFileStats(const std::string& filePath, float optionTextY)
{
    static std::string lastStatsPath;
    static std::string lastStatsLocale;
    static std::vector<StatsPanel::Row> statsRows;
    static OutfitFileStats fileStats;

    const std::string locale = Language::GetSelectedLocale();
    if (lastStatsPath != filePath || lastStatsLocale != locale)
    {
        lastStatsPath = filePath;
        lastStatsLocale = locale;
        fileStats = ReadOutfitFileStats(filePath);
        statsRows = BuildOutfitFileStatsRows(filePath, fileStats);
    }

    if (Previews::g_enabled && fileStats.modelHash != 0)
    {
        std::vector<StatsPanel::Row> rowsWithPreview;
        const std::string previewKey = "ped-outfit:" + filePath;
        const GTAmodel::Model pedModel(fileStats.modelHash);
        rowsWithPreview.push_back(Previews::CreateModelPreviewRow(previewKey, EntityType::PED, pedModel, filePath));
        rowsWithPreview.insert(rowsWithPreview.end(), statsRows.begin(), statsRows.end());
        StatsPanel::DrawAtOption(optionTextY, 0.100f, rowsWithPreview);
        return;
    }

    StatsPanel::DrawAtOption(optionTextY, 0.100f, statsRows);
}
} // namespace

namespace
{
bool DrawOutfitFileBrowser()
{
    std::string& directoryPath = outfitMenuState.directoryPath;
    std::string& searchText = outfitMenuState.searchText;
    std::vector<std::string> fileNames;

    if (directoryPath.empty())
        directoryPath = GetPathffA(Pathff::Outfit, false);

    DIR* dirPoint = opendir(directoryPath.c_str());
    dirent* entry = readdir(dirPoint);
    while (entry)
    {
        fileNames.push_back(entry->d_name);
        entry = readdir(dirPoint);
    }

    closedir(dirPoint);
    SortSaveFileEntries(fileNames, directoryPath);
    AddBreak(Language::Translate("menu.player.wardrobe.outfits.section.found_files.label"));

    bool folderBackPressed = false;
    AddOption("..", folderBackPressed);
    if (folderBackPressed)
    {
        directoryPath = directoryPath.substr(0, directoryPath.rfind("\\"));
        Menu::selectedOptionIndex = 6;
    }

    if (fileNames.empty())
        return false;

    bool searchPressed = false;
    AddOption(searchText.empty() ? Language::Translate("component.search.action.label") : searchText, searchPressed, nullFunc, -1, true);
    if (searchPressed)
    {
        searchText = Game::InputBox(searchText, 126U, Language::Translate("component.search.prompt").c_str(), boost::to_lower_copy(searchText));
        boost::to_upper(searchText);
    }

    for (const auto& fileName : fileNames)
    {
        if (fileName.front() == '.' || fileName.front() == ',')
            continue;
        if (!searchText.empty() && boost::to_upper_copy(fileName).find(searchText) == std::string::npos)
            continue;

        const bool isFolder = PathIsDirectoryA((directoryPath + "\\" + fileName).c_str()) != 0;
        const bool isXml = fileName.length() > 4 && fileName.rfind(".xml") == fileName.length() - 4;
        TICKOL icon = TICKOL::NONE;
        if (isFolder)
            icon = TICKOL::ARROWRIGHT;
        else if (isXml)
            icon = TICKOL::TICK2;
        bool filePressed = false;

        if (isFolder)
        {
            AddTickol(fileName + " >>>", true, filePressed, filePressed, icon, TICKOL::NONE);
            if (filePressed)
            {
                directoryPath = directoryPath + "\\" + fileName;
                Menu::selectedOptionIndex = 6;
            }

            if (Menu::IsLastDrawnOptionSelected() && !filePressed)
            {
                if (FolderPreviewBmps_catind::bFolderBmpsEnabled)
                    FolderPreviewBmps_catind::DrawBmp(directoryPath + "\\" + fileName);
                StatsPanel::DrawFolderContents(directoryPath + "\\" + fileName, currentOptionY + menuPos.y);
            }
        }
        else if (isXml)
        {
            AddTickol(fileName, true, filePressed, filePressed, icon, TICKOL::NONE);
            if (filePressed)
            {
                outfitMenuState.selectedOutfitName = fileName.substr(0, fileName.rfind('.'));
                outfitMenuState.selectedOutfitPath = directoryPath + "\\" + fileName;
                Menu::pendingSubmenu = SUB::PED_OUTFIT_OPTIONS;
                return true;
            }

            if (Menu::IsLastDrawnOptionSelected() && !filePressed)
                DrawOutfitFileStats(directoryPath + "\\" + fileName, currentOptionY + menuPos.y);
        }
    }

    return false;
}

void SaveOutfitFromMenu(const std::string& directoryPath)
{
    const std::string inputName = Game::InputBox("", 28U, "FMMC_KEY_TIP9");
    if (inputName.empty())
    {
        Game::Print::PrintErrorInvalidInput(inputName);
        return;
    }

    if (!IsSafePath(inputName))
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
        return;
    }

    const bool saved = PedOutfitPersistence::SavePedOutfit(g_activePedHandle, directoryPath + "\\" + inputName + ".xml", GetSelectedXmlFormat());
    if (saved)
        Game::Print::PrintBottomLeft(Language::Translate("message.player.wardrobe.file_created.message"));
    else
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.create_file_failed.message"));
}

void CreateOutfitFolder()
{
    std::string& directoryPath = outfitMenuState.directoryPath;
    const std::string folderName = Game::InputBox("", 28U, Language::Translate("menu.player.wardrobe.outfits.option.folder_prompt.prompt").c_str());
    if (folderName.empty())
    {
        Game::Print::PrintErrorInvalidInput(folderName);
        return;
    }

    if (!IsSafePath(folderName))
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
    }
    else if (CreateDirectoryA((directoryPath + "\\" + folderName).c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        directoryPath = directoryPath + "\\" + folderName;
        Menu::selectedOptionIndex = 6;
        Game::Print::PrintBottomLeft(Language::Translate("message.player.wardrobe.folder_created.message"));
    }
    else
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.failed.title"), Language::Translate("message.player.wardrobe.create_folder_failed.message"));
        addlog(ige::LogType::LOG_ERROR, "Attempt to create folder " + folderName + " failed");
    }
}
} // namespace

namespace
{
void SaveOutfitSettingsOrNotifyFailure(const std::string& filePath, const PedOutfitPersistence::OutfitFileSettings& settings)
{
    if (PedOutfitPersistence::SavePedOutfitSettings(filePath, settings))
        return;

    Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.overwrite_failed.message"));
}

void ApplySelectedOutfit(const std::string& filePath, bool applyModelAndHead, bool applyOtherAppearance)
{
    bool applyFailed = false;
    if (applyModelAndHead)
    {
        const bool applyingToPlayer = g_activePedHandle == PLAYER_PED_ID();
        if (!PedOutfitPersistence::ApplyPedOutfit(g_activePedHandle, filePath, PedOutfitPersistence::OutfitApplyOptions::ModelAndHeadOnly()))
            applyFailed = true;
        if (applyingToPlayer)
            g_activePedHandle = PLAYER_PED_ID();
    }

    if (applyOtherAppearance && !PedOutfitPersistence::ApplyPedOutfit(g_activePedHandle, filePath, PedOutfitPersistence::OutfitApplyOptions::AllExceptModelAndHead(GetAttachmentPersistenceMode())))
        applyFailed = true;

    if (applyFailed)
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.load_failed.message"));
}

void SaveSelectedOutfitAsDefault(const std::string& filePath)
{
    const bool modelAndHeadApplied = PedOutfitPersistence::ApplyPedOutfit(PLAYER_PED_ID(), filePath, PedOutfitPersistence::OutfitApplyOptions::ModelAndHeadOnly());
    const bool otherAppearanceApplied =
        PedOutfitPersistence::ApplyPedOutfit(PLAYER_PED_ID(), filePath, PedOutfitPersistence::OutfitApplyOptions::AllExceptModelAndHead(GetAttachmentPersistenceMode()));
    if (!modelAndHeadApplied || !otherAppearanceApplied)
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.spooner.files.load_failed.message"));
        return;
    }

    if (PedOutfitPersistence::SavePedOutfit(PLAYER_PED_ID(), "menyooStuff/defaultPed.xml", GetSelectedXmlFormat()))
        Game::Print::PrintBottomLeft(Language::Translate("message.player.wardrobe.outfit_default.message"));
    else
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.create_file_failed.message"));
        addlog(ige::LogType::LOG_ERROR, "Attempt to create file menyooStuff/defaultPed.xml failed");
    }
}

void OverwriteSelectedOutfit(const std::string& filePath)
{
    if (PedOutfitPersistence::SavePedOutfit(g_activePedHandle, filePath, GetSelectedXmlFormat()))
        Game::Print::PrintBottomLeft(Language::Translate("message.player.wardrobe.file_overwritten.message"));
    else
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.overwrite_failed.message"));
        addlog(ige::LogType::LOG_ERROR, "Attempt to overwrite file " + filePath + " failed");
    }
}

void RenameSelectedOutfit(const std::string& filePath)
{
    std::string& outfitName = outfitMenuState.selectedOutfitName;
    const std::string newName = Game::InputBox("", 28U, "FMMC_KEY_TIP9", outfitName);
    if (newName.empty())
    {
        Game::Print::PrintErrorInvalidInput(newName);
        return;
    }

    if (!IsSafePath(newName))
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.invalid_name.message"));
        return;
    }

    const std::string newPath = outfitMenuState.directoryPath + "\\" + newName + ".xml";
    const int renameResult = rename(filePath.c_str(), newPath.c_str());
    if (renameResult != 0)
    {
        addlog(ige::LogType::LOG_ERROR, "Failed to rename outfit file " + outfitName + " to " + newName);
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.rename_failed.message"));
        return;
    }

    outfitName = newName;
    outfitMenuState.selectedOutfitPath = newPath;
    Game::Print::PrintBottomLeft(Language::Translate("message.player.wardrobe.file_renamed.message"));
}

void DeleteSelectedOutfit(const std::string& filePath)
{
    if (remove(filePath.c_str()) == 0)
        Game::Print::PrintBottomLeft(Language::Translate("message.player.wardrobe.file_deleted.message"));
    else
    {
        Game::Print::ShowNotification(Language::Translate("message.player.wardrobe.error.title"), Language::Translate("message.player.wardrobe.delete_failed.message"));
        addlog(ige::LogType::LOG_ERROR, "Attempt to delete file " + filePath + " failed");
    }
    Menu::SetPreviousMenu();
    Menu::Up();
}

void DrawSelectedOutfitSettings(const std::string& filePath)
{
    PedOutfitPersistence::OutfitFileSettings settings;
    if (!PedOutfitPersistence::ReadPedOutfitSettings(filePath, settings))
        return;

    AddBreak(Language::Translate("menu.player.wardrobe.outfits.section.attributes.label"));
    bool clearDecalOverlaysPressed = false;
    AddTickol(
        Language::Translate("menu.player.wardrobe.outfits.option.clear_previous_decals.label"),
        settings.clearDecalOverlays,
        clearDecalOverlaysPressed,
        clearDecalOverlaysPressed,
        TICKOL::BOXTICK,
        TICKOL::BOXBLANK
    );
    AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.remove_decals.description"));
    if (clearDecalOverlaysPressed)
    {
        settings.clearDecalOverlays = !settings.clearDecalOverlays;
        settings.hasClearDecalOverlaysSetting = true;
        SaveOutfitSettingsOrNotifyFailure(filePath, settings);
    }

    if (settings.hasShortHeightSetting)
    {
        bool shortHeightPressed = false;
        AddTickol(Language::Translate("menu.player.wardrobe.outfits.option.short_height.label"), settings.shortHeight, shortHeightPressed, shortHeightPressed, TICKOL::BOXTICK, TICKOL::BOXBLANK);
        AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.short_ped.description"));
        if (shortHeightPressed)
        {
            settings.shortHeight = !settings.shortHeight;
            SaveOutfitSettingsOrNotifyFailure(filePath, settings);
        }
    }

    if (settings.hasAttachmentsPersistentSetting)
    {
        bool attachmentsPersistentPressed = false;
        AddTickol(
            Language::Translate("menu.player.wardrobe.outfits.option.persistent_attachments.label"),
            settings.attachmentsPersistent,
            attachmentsPersistentPressed,
            attachmentsPersistentPressed,
            TICKOL::BOXTICK,
            TICKOL::BOXBLANK
        );
        AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.attachments_persistent.description"));
        if (attachmentsPersistentPressed)
        {
            settings.attachmentsPersistent = !settings.attachmentsPersistent;
            SaveOutfitSettingsOrNotifyFailure(filePath, settings);
        }
    }

    if (settings.attachmentsPersistent && settings.hasStartTaskSequencesSetting)
    {
        bool startTaskSequencesPressed = false;
        AddTickol(
            Language::Translate("menu.player.wardrobe.outfits.option.start_task_sequences.label"),
            settings.startTaskSequencesOnLoad,
            startTaskSequencesPressed,
            startTaskSequencesPressed,
            TICKOL::BOXTICK,
            TICKOL::BOXBLANK
        );
        AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.attachments_tasks.description"));
        if (startTaskSequencesPressed)
        {
            settings.startTaskSequencesOnLoad = !settings.startTaskSequencesOnLoad;
            SaveOutfitSettingsOrNotifyFailure(filePath, settings);
        }
    }
}
} // namespace

namespace PedOutfitMenu
{
void ResetBrowserState()
{
    outfitMenuState.searchText.clear();
    outfitMenuState.directoryPath.clear();
    outfitMenuState.selectedOutfitPath.clear();
    outfitMenuState.selectedOutfitName.clear();
}
} // namespace PedOutfitMenu

namespace
{
void AddOutfitXmlFormatOption()
{
    outfitMenuState.useLegacyXmlFormat = AddTexterCycler(
                                             Language::Translate("menu.player.wardrobe.outfits.option.xml_format.label"),
                                             outfitMenuState.useLegacyXmlFormat,
                                             {Language::Translate("menu.player.wardrobe.option.outfits.value.xml_new"), Language::Translate("menu.player.wardrobe.option.outfits.value.xml_legacy")}
                                         ) == 1;
    AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.xml_format.description"));
}
} // namespace

void PedOutfitsMenu()
{
    std::uint8_t& attachmentModeIndex = outfitMenuState.attachmentPersistenceModeIndex;

    bool savePressed = false, createFolderPressed = false;

    AddTitle(Language::Translate("menu.player.wardrobe.outfits.title"));
    AddSaveFileSortOption();

    attachmentModeIndex = static_cast<std::uint8_t>(AddTexterCycler(
        Language::Translate("menu.player.wardrobe.outfits.option.attachments.label"),
        attachmentModeIndex,
        {Language::Translate("menu.player.wardrobe.option.outfits.value.file_decides"),
         Language::Translate("menu.player.wardrobe.option.outfits.value.force_off"),
         Language::Translate("menu.player.wardrobe.option.outfits.value.force_on")}
    ));
    AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.attachments.description"));

    AddOutfitXmlFormatOption();

    AddOption(Language::Translate("menu.player.wardrobe.outfits.option.save.label"), savePressed);

    AddOption(Language::Translate("menu.player.wardrobe.outfits.option.create_folder.label"), createFolderPressed);

    if (DrawOutfitFileBrowser())
        return;

    if (savePressed)
    {
        SaveOutfitFromMenu(outfitMenuState.directoryPath);
        return;
    }

    if (createFolderPressed)
    {
        CreateOutfitFolder();
        return;
    }
}

void PedOutfitOptionsMenu()
{
    const std::string filePath = outfitMenuState.selectedOutfitPath;
    const std::string& outfitName = outfitMenuState.selectedOutfitName;

    bool applyOutfitPressed = false;
    bool applyClothingPressed = false;
    bool applyModelAndHeadPressed = false;
    bool overwriteOutfitPressed = false;
    bool saveAsDefaultPressed = false;
    bool renameOutfitPressed = false;
    bool deleteOutfitPressed = false;

    AddTitle(outfitName);
    AddOption(Language::Translate("menu.player.wardrobe.outfits.option.apply.label"), applyOutfitPressed);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.apply.description"));
    AddOption(Language::Translate("menu.player.wardrobe.outfits.option.apply_clothing.label"), applyClothingPressed);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.apply_clothing.description"));
    AddOption(
        Language::Translate(g_activePedHandle == PLAYER_PED_ID() ? "menu.player.wardrobe.option.apply_ped_model.label" : "menu.player.wardrobe.option.apply_head_features.label"),
        applyModelAndHeadPressed
    );
    AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.apply_head.description"));
    AddOption(Language::Translate("menu.player.wardrobe.outfits.option.apply_default.label"), saveAsDefaultPressed);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.apply_default.description"));
    AddOutfitXmlFormatOption();
    AddOption(Language::Translate("menu.player.wardrobe.outfits.option.rename.label"), renameOutfitPressed);
    AddOption(Language::Translate("menu.player.wardrobe.outfits.option.overwrite.label"), overwriteOutfitPressed);
    AddOptionDescription(Language::Translate("menu.player.wardrobe.outfits.option.overwrite.description"));
    AddOption(Language::Translate("menu.player.wardrobe.outfits.option.delete.label"), deleteOutfitPressed);

    ApplySelectedOutfit(filePath, applyModelAndHeadPressed || applyOutfitPressed, applyClothingPressed || applyOutfitPressed);

    if (saveAsDefaultPressed)
        SaveSelectedOutfitAsDefault(filePath);

    if (overwriteOutfitPressed)
        OverwriteSelectedOutfit(filePath);

    if (renameOutfitPressed)
        RenameSelectedOutfit(filePath);

    if (deleteOutfitPressed)
    {
        DeleteSelectedOutfit(filePath);
        return;
    }

    DrawSelectedOutfitSettings(filePath);
}
} // namespace sub

#include "..\..\Menu\submenu_switch.h"
#include "..\..\Menu\submenu_enum.h"

REGISTER_SUBMENU(PED_OUTFITS, sub::PedOutfitsMenu)
REGISTER_SUBMENU(PED_OUTFIT_OPTIONS, sub::PedOutfitOptionsMenu)
