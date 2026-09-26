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

// First-chance crash observer. Installed only when the log level is at
// least DEBUG (see InitCrashHandler). Never swallows exceptions.
namespace ige
{
void InitCrashHandler();
void ShutdownCrashHandler();

// Records a native hash into the last-N ring buffer. Called from
// invoke<>() in Natives/nativeCaller.h. No-op unless the VEH is installed.
void RecordNative(unsigned long long nativeHash);
} // namespace ige
