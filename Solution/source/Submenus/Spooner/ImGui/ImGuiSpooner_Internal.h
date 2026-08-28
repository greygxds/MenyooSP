#pragma once

#include "ImGuiSpooner.h"

#include <mutex>

namespace sub::Spooner::ImGuiSpooner
{
	extern std::mutex g_Mutex;
	extern SharedState g_Shared;

	PopupRequest ProcessCursorCommands(const std::vector<QueuedCommand>& commands);
	void CheckPendingSpawns_ScriptThread();
}
