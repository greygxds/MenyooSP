/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
*/

#include "Keybinds.h"

#include "..\Menu\Language.h"
#include "..\Menu\submenu_enum.h"
#include "..\Menu\MenuCategory.h"

#include <cctype>

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
        std::string categoryKey = "keybinds.category.";
        for (const unsigned char character : category)
            categoryKey.push_back(static_cast<char>(std::tolower(character)));
        const std::string categoryLabel = Language::Translate(categoryKey);
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
    AddTitle(Language::Translate("keybinds.title"));
    AddOption(Language::Translate("keybinds.keyboard"), null, nullFunc, SUB::SETTINGS_KEYBINDS_KEYBOARD);
    AddOption(Language::Translate("keybinds.controller"), null, nullFunc, SUB::SETTINGS_KEYBINDS_GAMEPAD);
    AddOptionDescription(Language::Translate("keybinds.description"));
    int styleIdx =
        AddTexterCycler(Language::Translate("keybinds.gamepadLabels"), (int)Keybinds::gamepadLabelStyle, {Language::Translate("keybinds.xbox"), Language::Translate("keybinds.playstation")});
    if (styleIdx != (int)Keybinds::gamepadLabelStyle)
    {
        Keybinds::gamepadLabelStyle = (Keybinds::GamepadLabelStyle)styleIdx;
        Keybinds::Save();
    }
}

void KeybindsKeyboard()
{
    AddTitle(Language::Translate("keybinds.keyboard.title"));
    DrawBindCategories(Keybinds::Context::Keyboard);
}

void KeybindsGamepad()
{
    AddTitle(Language::Translate("keybinds.controller.title"));
    DrawBindCategories(Keybinds::Context::Gamepad);
}
} // namespace sub

#include "..\Menu\submenu_switch.h"

REGISTER_SUBMENU(SETTINGS_KEYBINDS, sub::KeybindsMenu)
REGISTER_SUBMENU(SETTINGS_KEYBINDS_KEYBOARD, sub::KeybindsKeyboard)
REGISTER_SUBMENU(SETTINGS_KEYBINDS_GAMEPAD, sub::KeybindsGamepad)
