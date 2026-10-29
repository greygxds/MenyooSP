/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "MainMenu.h"
#include "Settings.h"

namespace sub
{
void Settings()
{
    auto& bSyncWithConfig = MenuConfig::bSaveAtIntervals;

    AddTitle(Language::Translate("settings.title"));

    AddOption(Language::Translate("settings.keybinds.label"), null, nullFunc, SUB::SETTINGS_KEYBINDS);

    bool bChangeLangPressed = false;
    AddTexter(Language::Translate("settings.language.label"), 0, {Language::GetSelectedLangTitle()}, bChangeLangPressed);
    if (bChangeLangPressed)
    {
        Menu::pendingSubmenu = SUB::SETTINGS_LANGUAGE;
    }

    AddOption(Language::Translate("settings.themes.label"), null, nullFunc, SUB::SETTINGS_THEMES);
    AddOption(Language::Translate("settings.menuColours.label"), null, nullFunc, SUB::SETTINGS_COLOURS);
    AddOption(Language::Translate("settings.menuFonts.label"), null, nullFunc, SUB::SETTINGS_FONTS);
    AddOption(Language::Translate("settings.menuPosition.label"), null, nullFunc, SUB::SETTINGS_MENUPOS);
    AddOptionDescription(Language::Translate("settings.keybinds.description"));
    AddToggle(Language::Translate("settings.notificationBackground.label"), MenuConfig::bShowNotificationBackground, MenuConfig::SaveConfig, MenuConfig::SaveConfig);
    AddToggle(Language::Translate("settings.mouseSupport.label"), Menu::usingMouseInput);
    AddOptionDescription(Language::Translate("settings.mouseSupport.description"));
    AddToggle(Language::Translate("settings.gradients.label"), Menu::useGradientBackgrounds);
    AddOptionDescription(Language::Translate("settings.gradients.description"));
    AddToggle(Language::Translate("settings.titleboxGlobe.label"), Menu::enableGlareEffect);
    AddOptionDescription(Language::Translate("settings.titleboxGlobe.description"));
    AddToggle(Language::Translate("settings.centreTitle.label"), Menu::centerTitleText);
    AddToggle(Language::Translate("settings.centreOptions.label"), Menu::centerOptionText);
    AddToggle(Language::Translate("settings.centreBreaks.label"), Menu::centerBreakText);
    AddOptionDescription(Language::Translate("settings.centreBreaks.description"));
    AddToggle(Language::Translate("settings.optionTextStroke.label"), Menu::optionTextStroke);
    AddOptionDescription(Language::Translate("settings.optionTextStroke.description"));
    AddToggle(Language::Translate("settings.resetPlayerModel.label"), checkSelfDeathModel);
    AddOptionDescription(Language::Translate("settings.resetPlayerModel.description"));
    AddToggle(Language::Translate("settings.syncConfig.label"), bSyncWithConfig, MenuConfig::SaveConfig, MenuConfig::SaveConfig);
    AddOptionDescription(Language::Translate("settings.syncConfig.description"));
    AddToggle(Language::Translate("settings.lscPartSelection.label"), g_LSCCustoms);
    AddOptionDescription(Language::Translate("settings.lscPartSelection.description"));
    AddOption(Language::Translate("settings.resetToggles.label"), null, MenuConfig::ConfigResetHaxValues);
    AddOptionDescription(Language::Translate("settings.resetToggles.description"));
}

void SettingsMenuPos()
{
    bool settingsXPosPlus = false;
    bool settingsXPosMinus = false;
    bool settingsYPosPlus = false;
    bool settingsYPosMinus = false;

    AddTitle(Language::Translate("settings.menuPosition.title"));
    AddNumber(Language::Translate("settings.menuPosition.offsetX.label"), menuPos.x * 100 + 6.0, 2, null, settingsXPosPlus, settingsXPosMinus);
    AddNumber(Language::Translate("settings.menuPosition.offsetY.label"), menuPos.y * 100 + 7.4, 2, null, settingsYPosPlus, settingsYPosMinus);


    if (settingsXPosPlus)
    {
        if (menuPos.x < 0.7397f)
        {
            menuPos.x += 0.002f;
        }
        return;
    }
    else if (settingsXPosMinus)
    {
        if (menuPos.x > -0.0598f)
        {
            menuPos.x -= 0.002f;
        }
        if (menuPos.x < -0.0598f)
        {
            menuPos.x = -0.0598f;
        }
        return;
    }

    if (settingsYPosPlus)
    {
        if (menuPos.y < 0.85f)
        {
            menuPos.y += 0.002f;
        }
        return;
    }
    else if (settingsYPosMinus)
    {
        if (menuPos.y > -0.074f)
        {
            menuPos.y -= 0.002f;
        }
        if (menuPos.y < -0.074f)
        {
            menuPos.y = -0.074f;
        }
        return;
    }
}

RGBA* g_settingsRGBA;
int* g_settingsRGBA2;
INT8* g_settingsFont;
UINT8 settingsHUDColor = 0ui8;

void AddsettingscolOption(const std::string& text, RGBA& feature)
{
    auto& settings_rgba = g_settingsRGBA;

    bool pressed = false;
    AddOption(text, pressed, nullFunc, SUB::SETTINGS_COLOURS2);

    if (Menu::IsLastDrawnOptionSelected())
    {
        AddPresetColourOptionsPreviews(feature);
    }

    if (pressed)
    {
        settings_rgba = &feature;
    }
}

void SettingsColours()
{
    AddTitle(Language::Translate("settings.menuColours.title"));
    AddsettingscolOption(Language::Translate("settings.menuColours.titleBox.label"), titlebox);
    AddsettingscolOption(Language::Translate("settings.menuColours.background.label"), BG);
    AddsettingscolOption(Language::Translate("settings.menuColours.titleText.label"), titletext);
    AddsettingscolOption(Language::Translate("settings.menuColours.optionText.label"), optiontext);
    AddsettingscolOption(Language::Translate("settings.menuColours.selectedText.label"), selectedtext);
    AddsettingscolOption(Language::Translate("settings.menuColours.optionBreaks.label"), optionbreaks);
    AddsettingscolOption(Language::Translate("settings.menuColours.optionCount.label"), optioncount);
    AddsettingscolOption(Language::Translate("settings.menuColours.selectionBox.label"), selectionhi);
    AddsettingscolOption(Language::Translate("settings.menuColours.pedTrackers.label"), _globalPedTrackers_Col);
    AddToggle(Language::Translate("settings.menuColours.rainbow.label"), rainbowBoxes);
    AddOptionDescription(Language::Translate("settings.menuColours.rainbow.description"));
}

bool HexToRGBA(const std::string& hex, RGBA& colour)
{
    std::string h = hex;
    if (h.empty())
        return false;
    if (h[0] == '#')
        h = h.substr(1);
    if (h.length() != 6 && h.length() != 8)
        return false;
    for (char c : h)
        if (!isxdigit(c))
            return false;
    colour.R = std::stoi(h.substr(0, 2), nullptr, 16);
    colour.G = std::stoi(h.substr(2, 2), nullptr, 16);
    colour.B = std::stoi(h.substr(4, 2), nullptr, 16);
    colour.A = (h.length() == 8) ? std::stoi(h.substr(6, 2), nullptr, 16) : 255;
    return true;
}

void SettingsColours2()
{
    bool settingsRInput = false;
    bool settingsRPlus = false;
    bool settingsRMinus = false;
    bool settingsHUDColourApply = false;
    bool settingsHUDColourPlus = false;
    bool settingsHUDColourMinus = false;

    auto& settingsRGBA = g_settingsRGBA;
    auto& settingsRGBA2 = g_settingsRGBA2;

    AddTitle(Language::Translate("settings.menuColours.setColour.title"));
    AddNumber(Language::Translate("settings.menuColours.setColour.red.label"), settingsRGBA->R, 0, settingsRInput, settingsRPlus, settingsRMinus);

    switch (*Menu::activeOptionIndex)
    {
    case 1:
    case 2:
    case 3:
        AddPresetColourOptionsPreviews(settingsRGBA->R, settingsRGBA->G, settingsRGBA->B);
        break;
    }

    AddNumber(Language::Translate("settings.menuColours.setColour.green.label"), settingsRGBA->G, 0, settingsRInput, settingsRPlus, settingsRMinus);
    AddNumber(Language::Translate("settings.menuColours.setColour.blue.label"), settingsRGBA->B, 0, settingsRInput, settingsRPlus, settingsRMinus);
    AddNumber(Language::Translate("settings.menuColours.setColour.opacity.label"), settingsRGBA->A, 0, settingsRInput, settingsRPlus, settingsRMinus);
    AddTexter(Language::Translate("settings.menuColours.setColour.hudColour.label"), settingsHUDColor, HudColour::vHudColours, settingsHUDColourApply, settingsHUDColourPlus, settingsHUDColourMinus);
    AddOptionDescription(Language::Translate("settings.menuColours.setColour.hudColour.description"));

    {
        bool bHexInputPressed = false;
        AddOption(Language::Translate("settings.menuColours.setColour.inputHex.label"), bHexInputPressed);
        if (bHexInputPressed)
        {
            std::string input = Game::InputBox("", 10U, Language::Translate("settings.menuColours.setColour.inputHex.prompt").c_str(), "#");
            if (!HexToRGBA(input, *settingsRGBA))
                Game::Print::ShowNotification(Language::Translate("settings.menuColours.setColour.invalidHex.message"));
        }
    }

    AddBreak(Language::Translate("settings.menuColours.setColour.presets.break"));
    AddPresetColourOptions(settingsRGBA->R, settingsRGBA->G, settingsRGBA->B);

    switch (*Menu::activeOptionIndex)
    {
    case 1:
        settingsRGBA2 = &settingsRGBA->R;
        break;
    case 2:
        settingsRGBA2 = &settingsRGBA->G;
        break;
    case 3:
        settingsRGBA2 = &settingsRGBA->B;
        break;
    case 4:
        settingsRGBA2 = &settingsRGBA->A;
        break;
    }

    if (settingsRInput)
    {
        int tempHash = *settingsRGBA2;
        try
        {
            tempHash = abs(std::stoi(Game::InputBox(std::to_string(*settingsRGBA2), 4U, "", std::to_string(*settingsRGBA2))));
        }
        catch (...)
        {
            Game::Print::PrintErrorInvalidInput(std::to_string(tempHash));
        }

        if (!(tempHash >= 0 && tempHash <= 255))
        {
            Game::Print::PrintErrorInvalidInput(std::to_string(tempHash));
        }
        else
        {
            *settingsRGBA2 = tempHash;
        }
        return;
    }

    if (settingsRPlus)
    {
        if (*settingsRGBA2 < 255)
        {
            (*settingsRGBA2)++;
        }
        else
        {
            *settingsRGBA2 = 0;
        }
        return;
    }
    else if (settingsRMinus)
    {
        if (*settingsRGBA2 > 0)
        {
            (*settingsRGBA2)--;
        }
        else
        {
            *settingsRGBA2 = 255;
        }
        return;
    }

    if (settingsHUDColourPlus)
    {
        if (settingsHUDColor < HudColour::vHudColours.size() - 1)
        {
            settingsHUDColor++;
        }
        else
        {
            settingsHUDColor = 0;
        }
        return;
    }
    if (settingsHUDColourMinus)
    {
        if (settingsHUDColor > 0)
        {
            settingsHUDColor--;
        }
        else
        {
            settingsHUDColor = 180;
        }
        return;
    }
    if (settingsHUDColourApply)
    {
        GET_HUD_COLOUR(settingsHUDColor, &settingsRGBA->R, &settingsRGBA->G, &settingsRGBA->B, &inull);
        return;
    }
}

void AddsettingsfonOption(const std::string& text, int fontIndex, INT8& feature)
{
    auto& settingsFont = g_settingsFont;

    bool bitChangeFont = false;
    bool bitSetFeature = false;

    if (fontIndex == -1)
    {
        AddOption(text, bitSetFeature, nullFunc, SUB::SETTINGS_FONTS2);
    }
    else
    {
        AddOption(text, bitChangeFont);
    }

    if (bitSetFeature)
    {
        settingsFont = &feature;
    }
    else if (bitChangeFont)
    {
        *settingsFont = fontIndex;
    }
}

void SettingsFonts()
{
    AddTitle(Language::Translate("settings.menuFonts.title"));
    AddsettingsfonOption(Language::Translate("settings.menuFonts.titleFont.label"), -1, font_title);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.optionsFont.label"), -1, font_options);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.selectedOptionFont.label"), -1, font_selection);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.optionBreaksFont.label"), -1, font_breaks);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.hudFont.label"), -1, font_hud);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.speedoTextFont.label"), -1, font_speedo);
}

void SettingsFonts2()
{
    auto& settingsFont = g_settingsFont;
    bool fonts2Input = false;
    INT8 dummyFeature;

    AddTitle(Language::Translate("settings.menuFonts.setFont.title"));
    AddsettingsfonOption(Language::Translate("settings.menuFonts.normal.choice"), GTAfont::Arial, dummyFeature);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.impact.choice"), GTAfont::Impact, dummyFeature);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.italic.choice"), GTAfont::Italic, dummyFeature);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.pricedown.choice"), GTAfont::Pricedown, dummyFeature);
    AddsettingsfonOption(Language::Translate("settings.menuFonts.caps.choice"), GTAfont::Caps, dummyFeature);
    AddOption(Language::Translate("settings.menuFonts.inputIndex.label"), fonts2Input);
    AddOptionDescription(Language::Translate("settings.menuFonts.inputIndex.description"));

    if (fonts2Input)
    {
        std::string inputStr = Game::InputBox(std::to_string(*settingsFont), 7U, "", std::to_string(*settingsFont));
        auto& settings_font = g_settingsFont;
        int tempHash = *settings_font;
        try
        {
            tempHash = abs(stoi(inputStr));
        }
        catch (...)
        {
            Game::Print::PrintErrorInvalidInput(inputStr);
        }
        *settings_font = tempHash;
    }
}

void SettingsLanguage()
{
    AddTitle(Language::Translate("settings.language.title"));
    AddTickol(Language::GetLocaleTitle("en-US"), Language::GetSelectedLocale() == "en-US", Language::ResetSelectedLang, Language::ResetSelectedLang);

    for (const auto& locale : Language::GetAvailableLocales())
    {
        if (locale.localeTag == "en-US")
            continue;
        bool bPressed = false;
        AddTickol(locale.displayName, Language::GetSelectedLocale() == locale.localeTag, bPressed, bPressed);
        if (bPressed)
        {
            Language::SetSelectedLangFromString(locale.localeTag);
        }
    }

    AddTickol(Language::Translate("settings.language.reload.label"), true, Language::Reload, Language::Reload, TICKOL::CROSS);
}

class MenyooTheme
{
  private:
    bool grads;
    bool rainbow;
    bool thinLineOverFooter;

    RGBA ttbox;
    RGBA bgbox;
    RGBA tttext;
    RGBA optext;
    RGBA seltext;
    RGBA opbreak;
    RGBA opcount;
    RGBA selhi;
    RGBA pedtrackers;

    INT8 f_title;
    INT8 f_options;
    INT8 f_selection;
    INT8 f_breaks;
    INT8 f_hud;
    INT8 f_speedo;

  public:
    MenyooTheme()
    {
    }

    MenyooTheme(
        bool _grads,
        bool _rainbow,
        bool _thinLineOverFooter,
        RGBA _ttbox,
        RGBA _bgbox,
        RGBA _tttext,
        RGBA _optext,
        RGBA _seltext,
        RGBA _opbreak,
        RGBA _opcount,
        RGBA _selhi,
        RGBA _pedtrackers,
        INT8 _f_title,
        INT8 _f_options,
        INT8 _f_selection,
        INT8 _f_breaks,
        INT8 _f_hud,
        INT8 _f_speedo
    )
    {
        grads = _grads;
        rainbow = _rainbow;
        thinLineOverFooter = _thinLineOverFooter;

        ttbox = _ttbox;
        bgbox = _bgbox;
        tttext = _tttext;
        optext = _optext;
        seltext = _seltext;
        opbreak = _opbreak;
        opcount = _opcount;
        selhi = _selhi;
        pedtrackers = _pedtrackers;

        f_title = _f_title;
        f_options = _f_options;
        f_selection = _f_selection;
        f_breaks = _f_breaks;
        f_hud = _f_hud;
        f_speedo = _f_speedo;
    }

    void SetActive()
    {
        Menu::useGradientBackgrounds = grads;
        rainbowBoxes = rainbow;
        Menu::drawSeparatorLine = thinLineOverFooter;

        titlebox = ttbox;
        BG = bgbox;
        titletext = tttext;
        optiontext = optext;
        selectedtext = seltext;
        optionbreaks = opbreak;
        optioncount = opcount;
        selectionhi = selhi;
        _globalPedTrackers_Col = pedtrackers;

        font_title = f_title;
        font_options = f_options;
        font_selection = f_selection;
        font_breaks = f_breaks;
        font_hud = f_hud;
        font_speedo = f_speedo;
    }

    bool IsActive()
    {
        return Menu::useGradientBackgrounds == grads && rainbowBoxes == rainbow && Menu::drawSeparatorLine == thinLineOverFooter &&

               titlebox == ttbox && BG == bgbox && titletext == tttext && optiontext == optext && selectedtext == seltext && optionbreaks == opbreak && optioncount == opcount &&
               selectionhi == selhi && _globalPedTrackers_Col == pedtrackers &&

               font_title == f_title && font_options == f_options && font_selection == f_selection && font_breaks == f_breaks && font_hud == f_hud && font_speedo == f_speedo;
    }

    static MenyooTheme CurrentlyActiveTheme()
    {
        MenyooTheme curr;

        curr.grads = Menu::useGradientBackgrounds;
        curr.rainbow = rainbowBoxes;
        curr.thinLineOverFooter = Menu::drawSeparatorLine;

        curr.ttbox = titlebox;
        curr.bgbox = BG;
        curr.tttext = titletext;
        curr.optext = optiontext;
        curr.seltext = selectedtext;
        curr.opbreak = optionbreaks;
        curr.opcount = optioncount;
        curr.selhi = selectionhi;
        curr.pedtrackers = _globalPedTrackers_Col;

        curr.f_title = font_title;
        curr.f_options = font_options;
        curr.f_selection = font_selection;
        curr.f_breaks = font_breaks;
        curr.f_hud = font_hud;
        curr.f_speedo = font_speedo;

        return curr;
    }

    bool operator==(const MenyooTheme& value2) const
    {
        const MenyooTheme& value1 = *this;

        return value1.grads == value2.grads && value1.rainbow == value2.rainbow && value1.thinLineOverFooter == value2.thinLineOverFooter &&

               value1.ttbox == value2.ttbox && value1.bgbox == value2.bgbox && value1.tttext == value2.tttext && value1.optext == value2.optext && value1.seltext == value2.seltext &&
               value1.opbreak == value2.opbreak && value1.opcount == value2.opcount && value1.selhi == value2.selhi && value1.pedtrackers == value2.pedtrackers &&

               value1.f_title == value2.f_title && value1.f_options == value2.f_options && value1.f_selection == value2.f_selection && value1.f_breaks == value2.f_breaks &&
               value1.f_hud == value2.f_hud && value1.f_speedo == value2.f_speedo;
    }

    bool Equals(MenyooTheme const& value2)
    {
        return (this->operator==(value2));
    }
};

class MenyooThemeNamed
{
  public:
    std::string name;
    MenyooTheme theme;

    MenyooThemeNamed(const std::string& newName, const MenyooTheme& newTheme)
    {
        this->name = newName;
        this->theme = newTheme;
    }
};

std::vector<MenyooThemeNamed> vValues_MenyooThemes{
    {"Black & Green",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(0, 0, 0, 255),
         RGBA(0, 0, 0, 100),
         RGBA(0, 255, 100, 255),
         RGBA(0, 255, 100, 255),
         RGBA(255, 255, 255, 255),
         RGBA(0, 255, 100, 255),
         RGBA(0, 255, 100, 255),
         RGBA(0, 255, 100, 255),
         RGBA(0, 255, 100, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"Black & Teal",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(0, 0, 0, 255),
         RGBA(0, 0, 0, 150),
         RGBA(0, 255, 255, 255),
         RGBA(0, 255, 255, 255),
         RGBA(255, 255, 255, 255),
         RGBA(0, 255, 255, 255),
         RGBA(0, 255, 255, 255),
         RGBA(0, 255, 255, 150),
         RGBA(0, 255, 255, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"Black & Red",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(0, 0, 0, 255),
         RGBA(0, 0, 0, 175),
         RGBA(255, 0, 0, 255),
         RGBA(255, 0, 0, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 0, 0, 255),
         RGBA(255, 0, 0, 255),
         RGBA(255, 0, 0, 100),
         RGBA(255, 0, 0, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"Black & White",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(0, 0, 0, 255),
         RGBA(0, 0, 0, 50),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 255, 150),
         RGBA(255, 255, 255, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"Black & Yellow",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(0, 0, 0, 255),
         RGBA(0, 0, 0, 100),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 100, 125),
         RGBA(255, 255, 100, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"White & Green",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(255, 255, 255, 255),
         RGBA(0, 0, 0, 100),
         RGBA(0, 255, 100, 255),
         RGBA(0, 255, 100, 255),
         RGBA(255, 255, 255, 255),
         RGBA(0, 255, 100, 255),
         RGBA(0, 255, 100, 255),
         RGBA(0, 255, 125, 150),
         RGBA(0, 255, 100, 255),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"White & Teal",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(255, 255, 255, 255),
         RGBA(0, 0, 0, 100),
         RGBA(0, 255, 255, 255),
         RGBA(0, 255, 255, 255),
         RGBA(255, 255, 255, 255),
         RGBA(0, 255, 255, 255),
         RGBA(0, 255, 255, 255),
         RGBA(0, 255, 255, 150),
         RGBA(0, 255, 255, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"White & Red",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(255, 255, 255, 255),
         RGBA(0, 0, 0, 175),
         RGBA(255, 0, 0, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 0, 0, 255),
         RGBA(255, 0, 0, 255),
         RGBA(255, 0, 0, 150),
         RGBA(255, 0, 0, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"White & Black",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(255, 255, 255, 255),
         RGBA(0, 0, 0, 150),
         RGBA(0, 0, 0, 255),
         RGBA(255, 255, 255, 255),
         RGBA(0, 0, 0, 255),
         RGBA(0, 0, 0, 255),
         RGBA(0, 0, 0, 255),
         RGBA(255, 255, 255, 150),
         RGBA(255, 255, 255, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"White & Yellow",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(255, 255, 255, 255),
         RGBA(0, 0, 0, 100),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 100, 255),
         RGBA(255, 255, 100, 125),
         RGBA(255, 255, 100, 205),
         GTAfont::Italic,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"Halloween",
     MenyooTheme(
         true,
         false,
         false,
         RGBA(22, 161, 18, 255),
         RGBA(96, 62, 148, 170),
         RGBA(255, 51, 0, 255),
         RGBA(255, 255, 255, 255),
         RGBA(0, 0, 0, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 51, 0, 255),
         RGBA(255, 51, 0, 150),
         RGBA(22, 161, 18, 205),
         GTAfont::Pricedown,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
    {"Elegant Purple",
     MenyooTheme(
         true,
         false,
         true,
         RGBA(102, 0, 204, 255),
         RGBA(10, 10, 10, 255),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 255, 255),
         RGBA(0, 0, 0, 255),
         RGBA(255, 255, 255, 240),
         RGBA(255, 255, 255, 255),
         RGBA(255, 255, 255, 211),
         RGBA(153, 51, 255, 205),
         GTAfont::Pricedown,
         GTAfont::Impact,
         GTAfont::Impact,
         GTAfont::Italic,
         GTAfont::Arial,
         GTAfont::Pricedown
     )},
};

void SettingsThemesMain()
{
    AddTitle(Language::Translate("settings.themes.title"));

    bool pressed = false;
    for (auto& them : vValues_MenyooThemes)
    {
        pressed = false;
        BOOL isActive = (them.theme == MenyooTheme::CurrentlyActiveTheme()) ? TRUE : FALSE;
        AddTickol(them.name, isActive, pressed, pressed, TICKOL::MAKEUPTHING, TICKOL::NONE, false);
        if (pressed)
        {
            them.theme.SetActive();
        }
    }
}
} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(SETTINGS, sub::Settings)
REGISTER_SUBMENU(SETTINGS_MENUPOS, sub::SettingsMenuPos)
REGISTER_SUBMENU(SETTINGS_THEMES, sub::SettingsThemesMain)
REGISTER_SUBMENU(SETTINGS_COLOURS, sub::SettingsColours)
REGISTER_SUBMENU(SETTINGS_COLOURS2, sub::SettingsColours2)
REGISTER_SUBMENU(SETTINGS_FONTS, sub::SettingsFonts)
REGISTER_SUBMENU(SETTINGS_FONTS2, sub::SettingsFonts2)
REGISTER_SUBMENU(SETTINGS_LANGUAGE, sub::SettingsLanguage)
