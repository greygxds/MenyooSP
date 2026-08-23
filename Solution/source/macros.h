/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#pragma once

//#pragma comment(lib, "$(SolutionDir)\external\ScriptHookV.lib")

#pragma warning(disable : 4244 4305) // double <-> float conversions

#define _CRT_SECURE_NO_WARNINGS

#if __has_include("build_version.h")
#include "build_version.h"
#else
#define MENYOO_BUILD_NUMBER 0
#define MENYOO_VERSION_TEXT "Development"
#define MENYOO_COMMIT_SHA "local"
#endif

#define GAME_PLAYERCOUNT 30








