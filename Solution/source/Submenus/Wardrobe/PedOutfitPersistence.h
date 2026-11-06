#pragma once

#include <string>

class GTAped;

namespace sub::PedOutfitPersistence
{
enum class OutfitXmlFormat
{
    Readable,
    Legacy
};

enum class AttachmentPersistenceMode
{
    UseFileSetting,
    ForceOff,
    ForceOn
};

struct OutfitApplyOptions
{
    bool applyModelAndHead = false;
    bool applyProps = false;
    bool applyComponents = false;
    bool applyDecals = false;
    bool applyDamageTextures = false;
    bool applyAttachments = false;
    AttachmentPersistenceMode attachmentPersistence = AttachmentPersistenceMode::UseFileSetting;

    static OutfitApplyOptions ModelAndHeadOnly();
    static OutfitApplyOptions AllExceptModelAndHead(AttachmentPersistenceMode attachmentPersistence = AttachmentPersistenceMode::UseFileSetting);
    static OutfitApplyOptions CompleteWithoutAttachments();
};

struct OutfitFileSettings
{
    bool clearDecalOverlays = true;
    bool hasClearDecalOverlaysSetting = false;
    bool shortHeight = false;
    bool hasShortHeightSetting = false;
    bool attachmentsPersistent = false;
    bool hasAttachmentsPersistentSetting = false;
    bool startTaskSequencesOnLoad = true;
    bool hasStartTaskSequencesSetting = false;
};

bool SavePedOutfit(GTAped ped, const std::string& filePath, OutfitXmlFormat format = OutfitXmlFormat::Readable);
bool ApplyPedOutfit(GTAped ped, const std::string& filePath, const OutfitApplyOptions& options);
bool ReadPedOutfitSettings(const std::string& filePath, OutfitFileSettings& settings);
bool SavePedOutfitSettings(const std::string& filePath, const OutfitFileSettings& settings);
} // namespace sub::PedOutfitPersistence
