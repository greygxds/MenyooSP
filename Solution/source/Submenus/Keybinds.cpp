/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
*/

#include "Keybinds.h"

#include "..\Menu\Language.h"
#include "..\Menu\submenu_enum.h"
#include "..\Menu\MenuCategory.h"

namespace sub
{
namespace
{
void DrawBindCategories(Keybinds::Context context)
{
    for (const std::string& category : Keybinds::Categories())
    {
        int count = 0;
        for (const auto& entry : Keybinds::Registry())
            if (entry.Category() == category)
                count++;
        const std::string categoryLabel = Language::TranslateStable("keybind.category", category, "label", category);
        std::string catLabel = std::string("— ~b~") + categoryLabel + "~s~ ~c~(" + std::to_string(count) + " binds)~s~";

        if (!MenuCategory::AddCategory(catLabel))
            continue;
        for (const auto& entry : Keybinds::Registry())
        {
            if (entry.Category() != category)
                continue;
            bool pressed = false;
            bool rebindingThis = Keybinds::IsRebinding(&entry, context);
            AddKeybindOption(entry.Label(), Keybinds::ColorizeRowText(entry, context), entry.Description(), pressed, rebindingThis);
            if (pressed)
                Keybinds::StartRebind(entry, context);
        }
    }
}
} // namespace

void KeybindsMenu()
{
    AddTitle(Language::Translate("menu.settings.keybinds.title"));
    AddOption(Language::Translate("menu.settings.keybinds.option.keyboard.label"), null, nullFunc, SUB::SETTINGS_KEYBINDS_KEYBOARD);
    AddOption(Language::Translate("menu.settings.keybinds.option.controller.label"), null, nullFunc, SUB::SETTINGS_KEYBINDS_GAMEPAD);
    AddOptionDescription(Language::Translate("menu.settings.keybinds.option.controls.description"));
    int styleIdx = AddTexterCycler(
        Language::Translate("menu.settings.keybinds.option.gamepad_labels.label"),
        (int)Keybinds::gamepadLabelStyle,
        {Language::Translate("menu.settings.keybinds.option.gamepad_labels.value.xbox"), Language::Translate("menu.settings.keybinds.option.gamepad_labels.value.playstation")}
    );
    if (styleIdx != (int)Keybinds::gamepadLabelStyle)
    {
        Keybinds::gamepadLabelStyle = (Keybinds::GamepadLabelStyle)styleIdx;
        Keybinds::Save();
    }
}

void KeybindsKeyboard()
{
    AddTitle(Language::Translate("menu.settings.keybinds.keyboard.title"));
    DrawBindCategories(Keybinds::Context::Keyboard);
}

void KeybindsGamepad()
{
    AddTitle(Language::Translate("menu.settings.keybinds.controller.title"));
    DrawBindCategories(Keybinds::Context::Gamepad);
}
} // namespace sub

#include "..\Menu\submenu_switch.h"

REGISTER_SUBMENU(SETTINGS_KEYBINDS, sub::KeybindsMenu)
REGISTER_SUBMENU(SETTINGS_KEYBINDS_KEYBOARD, sub::KeybindsKeyboard)
REGISTER_SUBMENU(SETTINGS_KEYBINDS_GAMEPAD, sub::KeybindsGamepad)
