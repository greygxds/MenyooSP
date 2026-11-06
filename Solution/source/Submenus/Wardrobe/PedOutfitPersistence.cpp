#include "PedOutfitPersistence.h"

#include "..\..\macros.h"
#include "..\..\Menu\Routine.h"
#include "..\..\Natives\natives2.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\enums.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Scripting\Game.h"
#include "..\PedModelChanger.h"
#include "..\Spooner\Databases.h"
#include "..\Spooner\EntityManagement.h"
#include "..\Spooner\FileManagement.h"
#include "..\Spooner\SpoonerEntity.h"
#include "..\Spooner\SpoonerMode.h"

#include <pugixml\src\pugixml.hpp>

#include <string>
#include <unordered_set>
#include <vector>

namespace sub::PedOutfitPersistence
{
OutfitApplyOptions OutfitApplyOptions::ModelAndHeadOnly()
{
    OutfitApplyOptions options;
    options.applyModelAndHead = true;
    return options;
}

OutfitApplyOptions OutfitApplyOptions::AllExceptModelAndHead(AttachmentPersistenceMode attachmentPersistence)
{
    OutfitApplyOptions options;
    options.applyProps = true;
    options.applyComponents = true;
    options.applyDecals = true;
    options.applyDamageTextures = true;
    options.applyAttachments = true;
    options.attachmentPersistence = attachmentPersistence;
    return options;
}

OutfitApplyOptions OutfitApplyOptions::CompleteWithoutAttachments()
{
    OutfitApplyOptions options;
    options.applyModelAndHead = true;
    options.applyProps = true;
    options.applyComponents = true;
    options.applyDecals = true;
    options.applyDamageTextures = true;
    return options;
}

bool SavePedOutfit(GTAped ped, const std::string& filePath, OutfitXmlFormat format)
{
    Spooner::SpoonerEntity pedEntity;
    pedEntity.handle = ped;
    pedEntity.type = EntityType::PED;
    pedEntity.dynamic = true;

    bool clearDecalOverlays = true;
    bool attachmentsPersistent = false;
    bool startTaskSequencesOnLoad = true;

    pugi::xml_document existingDocument;
    if (existingDocument.load_file(filePath.c_str()).status == pugi::status_ok)
    {
        const auto existingRoot = existingDocument.child("OutfitPedData");
        clearDecalOverlays = existingRoot.child("ClearDecalOverlays").text().as_bool(clearDecalOverlays);
        const auto existingAttachments = existingRoot.child("SpoonerAttachments");
        attachmentsPersistent = existingAttachments.attribute("SetAttachmentsPersistentAndAddToSpoonerDatabase").as_bool(attachmentsPersistent);
        startTaskSequencesOnLoad = existingAttachments.attribute("StartTaskSequencesOnLoad").as_bool(startTaskSequencesOnLoad);
    }

    const bool useLegacyXmlFormat = format == OutfitXmlFormat::Legacy;
    pugi::xml_document document;
    auto declaration = document.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "ISO-8859-1";

    auto outfitRoot = document.append_child("OutfitPedData");
    outfitRoot.append_child("ClearDecalOverlays").text() = clearDecalOverlays;
    Spooner::FileManagement::AddEntityToXmlNode(pedEntity, outfitRoot, useLegacyXmlFormat);

    auto attachmentsNode = outfitRoot.append_child("SpoonerAttachments");
    attachmentsNode.append_attribute("SetAttachmentsPersistentAndAddToSpoonerDatabase") = attachmentsPersistent;
    attachmentsNode.append_attribute("StartTaskSequencesOnLoad") = startTaskSequencesOnLoad;
    for (auto& databaseEntity : Spooner::Databases::EntityDb)
    {
        if (!databaseEntity.attachmentArgs.isAttached)
            continue;

        GTAentity attachedTo;
        if (!Spooner::EntityManagement::GetEntityThisEntityIsAttachedTo(databaseEntity.handle, attachedTo))
            continue;
        if (attachedTo.Handle() != ped.Handle())
            continue;

        auto attachmentNode = attachmentsNode.append_child("Attachment");
        Spooner::FileManagement::AddEntityToXmlNode(databaseEntity, attachmentNode, useLegacyXmlFormat);
    }

    return document.save_file(filePath.c_str());
}

bool ApplyPedOutfit(GTAped ped, const std::string& filePath, const OutfitApplyOptions& options)
{
    pugi::xml_document document;
    if (document.load_file(filePath.c_str()).status != pugi::status_ok)
        return false;

    const bool networkGameInProgress = NETWORK::NETWORK_IS_GAME_IN_PROGRESS() != 0;
    const auto outfitRoot = document.child("OutfitPedData");
    ped.RequestControl(400);

    const Model pedModel = outfitRoot.child("ModelHash").text().as_uint();
    const auto pedProperties = outfitRoot.child("PedProperties");

    if (options.applyModelAndHead)
    {
        if (ped.Handle() == PLAYER_PED_ID())
        {
            const bool wasActivePed = g_activePedHandle == ped.Handle();
            ChangeModel(pedModel);
            ped = PLAYER_PED_ID();
            if (wasActivePed)
                g_activePedHandle = ped.Handle();
        }

        if (pedProperties.child("HasShortHeight").text().as_bool())
            SET_PED_CONFIG_FLAG(ped.Handle(), ePedConfigFlags::_Shrink, true);

        Spooner::FileManagement::LoadPedHeadFeaturesFromXml(ped, pedProperties.child("HeadFeatures"), pedModel);

        const auto facialMood = pedProperties.child("FacialMood");
        if (facialMood)
            SetPedFacialMood(ped, facialMood.text().as_string());

        const int opacityLevel = outfitRoot.child("OpacityLevel").text().as_int(255);
        if (opacityLevel < 255)
            ped.SetAlpha(opacityLevel);
        ped.SetVisible(outfitRoot.child("IsVisible").text().as_bool());
    }

    if (outfitRoot.child("ClearDecalOverlays").text().as_bool(true))
        ped.ClearDecorations();
    ped.ClearTrackedDecorations();
    if (options.applyDecals)
        Spooner::FileManagement::LoadPedDecalsFromXml(ped, pedProperties.child("TattooLogoDecals"));

    if (options.applyComponents)
        Spooner::FileManagement::LoadPedCompsFromXml(ped, pedProperties.child("PedComps"));

    if (options.applyProps)
    {
        CLEAR_ALL_PED_PROPS(ped.Handle(), 0);
        Spooner::FileManagement::LoadPedPropsFromXml(ped, pedProperties.child("PedProps"), networkGameInProgress);
    }

    ped.ClearBloodDamage();
    ped.ResetVisibleDamage();
    if (options.applyDamageTextures)
    {
        ped.ClearTrackedDamagePacks();
        Spooner::FileManagement::LoadPedDamagePacksFromXml(ped, pedProperties.child("DamagePacks"));
    }

    if (options.applyAttachments)
    {
        std::unordered_set<Hash> modelHashes;
        std::vector<Spooner::SpoonerEntityWithInitHandle> spawnedAttachments;
        const auto attachmentsNode = outfitRoot.child("SpoonerAttachments");
        bool addAttachmentsToSpoonerDatabase = attachmentsNode.attribute("SetAttachmentsPersistentAndAddToSpoonerDatabase").as_bool(false);
        const bool startTaskSequencesOnLoad = attachmentsNode.attribute("StartTaskSequencesOnLoad").as_bool(true);

        switch (options.attachmentPersistence)
        {
        case AttachmentPersistenceMode::UseFileSetting:
            break;
        case AttachmentPersistenceMode::ForceOff:
            addAttachmentsToSpoonerDatabase = false;
            break;
        case AttachmentPersistenceMode::ForceOn:
            addAttachmentsToSpoonerDatabase = true;
            break;
        }

        for (auto attachmentNode = attachmentsNode.first_child(); attachmentNode; attachmentNode = attachmentNode.next_sibling())
        {
            auto spawnedAttachment = Spooner::FileManagement::SpawnEntityFromXmlNode(attachmentNode, modelHashes);
            Spooner::EntityManagement::AttachEntity(
                spawnedAttachment.e, ped, spawnedAttachment.e.attachmentArgs.boneIndex, spawnedAttachment.e.attachmentArgs.offset, spawnedAttachment.e.attachmentArgs.rotation
            );
            spawnedAttachments.push_back(spawnedAttachment);

            if (addAttachmentsToSpoonerDatabase)
            {
                if (!spawnedAttachment.e.taskSequence.empty())
                {
                    auto& tasks = spawnedAttachment.e.taskSequence.AllTasks();
                    for (auto& previousAttachment : spawnedAttachments)
                    {
                        for (auto& task : tasks)
                            task->LoadTargetingDressing(previousAttachment.initHandle, previousAttachment.e.handle.Handle());
                    }
                    if (startTaskSequencesOnLoad)
                        spawnedAttachment.e.taskSequence.Start();
                }
                Spooner::Databases::EntityDb.push_back(spawnedAttachment.e);
            }
            else
            {
                spawnedAttachment.e.handle.NoLongerNeeded();
            }
        }

        for (const auto modelHash : modelHashes)
            Model(modelHash).Unload();
    }

    return true;
}

bool ReadPedOutfitSettings(const std::string& filePath, OutfitFileSettings& settings)
{
    settings = OutfitFileSettings{};
    pugi::xml_document document;
    if (document.load_file(filePath.c_str()).status != pugi::status_ok)
        return false;

    const auto outfitRoot = document.child("OutfitPedData");
    if (!outfitRoot)
        return false;

    const auto clearDecalOverlays = outfitRoot.child("ClearDecalOverlays");
    settings.hasClearDecalOverlaysSetting = static_cast<bool>(clearDecalOverlays);
    settings.clearDecalOverlays = clearDecalOverlays.text().as_bool(true);

    const auto pedProperties = outfitRoot.child("PedProperties");
    const auto shortHeight = pedProperties.child("HasShortHeight");
    settings.hasShortHeightSetting = static_cast<bool>(shortHeight);
    settings.shortHeight = shortHeight.text().as_bool();

    const auto attachments = outfitRoot.child("SpoonerAttachments");
    const auto persistentAttachments = attachments.attribute("SetAttachmentsPersistentAndAddToSpoonerDatabase");
    settings.hasAttachmentsPersistentSetting = static_cast<bool>(persistentAttachments);
    settings.attachmentsPersistent = persistentAttachments.as_bool();
    const auto startTaskSequences = attachments.attribute("StartTaskSequencesOnLoad");
    settings.hasStartTaskSequencesSetting = static_cast<bool>(startTaskSequences);
    settings.startTaskSequencesOnLoad = startTaskSequences.as_bool();
    return true;
}

bool SavePedOutfitSettings(const std::string& filePath, const OutfitFileSettings& settings)
{
    pugi::xml_document document;
    if (document.load_file(filePath.c_str()).status != pugi::status_ok)
        return false;

    auto outfitRoot = document.child("OutfitPedData");
    if (!outfitRoot)
        return false;

    if (settings.hasClearDecalOverlaysSetting)
    {
        auto clearDecalOverlays = outfitRoot.child("ClearDecalOverlays");
        if (!clearDecalOverlays)
            clearDecalOverlays = outfitRoot.append_child("ClearDecalOverlays");
        clearDecalOverlays.text() = settings.clearDecalOverlays;
    }

    if (settings.hasShortHeightSetting)
        outfitRoot.child("PedProperties").child("HasShortHeight").text() = settings.shortHeight;

    const auto attachments = outfitRoot.child("SpoonerAttachments");
    if (settings.hasAttachmentsPersistentSetting)
        attachments.attribute("SetAttachmentsPersistentAndAddToSpoonerDatabase") = settings.attachmentsPersistent;
    if (settings.hasStartTaskSequencesSetting)
        attachments.attribute("StartTaskSequencesOnLoad") = settings.startTaskSequencesOnLoad;

    return document.save_file(filePath.c_str());
}
} // namespace sub::PedOutfitPersistence
