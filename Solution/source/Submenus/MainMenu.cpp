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
#include "..\Menu\Language.h"

namespace sub
{
void MainMenu()
{
    static bool firstLoad = true;
    if (firstLoad)
    {
        firstLoad = false;
        addlog(ige::LogType::LOG_TRACE, "First Load of MainMenu Submenu");
    }

    AddTitle(Language::Translate("menu.main.title"));
    AddOption(Language::Translate("menu.main.players.label"), null, nullFunc, SUB::PLAYERSSUB);
    AddOption(Language::Translate("menu.main.playerOptions.label"), null, nullFunc, SUB::PLAYEROPS);
    AddOption(Language::Translate("menu.main.vehicleOptions.label"), null, nullFunc, SUB::VEHICLEOPS);
    AddOption(Language::Translate("menu.main.teleportOptions.label"), null, nullFunc, SUB::TELEPORTOPS);
    AddOption(Language::Translate("menu.main.weaponOptions.label"), null, nullFunc, SUB::WEAPONOPS);
    AddOption(Language::Translate("menu.main.weatherOptions.label"), null, nullFunc, SUB::WEATHEROPS);
    AddOption(Language::Translate("menu.main.timeOptions.label"), null, nullFunc, SUB::TIMEOPS);
    AddOption(Language::Translate("menu.main.objectSpooner.label"), null, nullFunc, SUB::SPOONER_MAIN);
    AddOption(Language::Translate("menu.main.miscOptions.label"), null, nullFunc, SUB::MISCOPS);
    AddOption(Language::Translate("menu.main.settings.label"), null, nullFunc, SUB::SETTINGS);
    AddOption(Language::Translate("menu.main.peopleBehindMenyoo.label"), null, nullFunc, SUB::CREDITSSUB);

    g_activePlayerId = PLAYER_ID();
    g_activePedHandle = PLAYER_PED_ID();
    g_playerName = GET_PLAYER_NAME(g_activePlayerId);
    g_playerGroupId = GET_PLAYER_GROUP(g_activePlayerId);
}
} // namespace sub

#include "..\Menu\submenu_switch.h"
#include "..\Menu\submenu_enum.h"
REGISTER_SUBMENU(MAINMENU, sub::MainMenu)
