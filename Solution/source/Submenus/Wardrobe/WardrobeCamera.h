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

class GTAped;

namespace sub
{
namespace WardrobeCamera
{
enum class Framing : unsigned char
{
    Body,
    Head
};

bool IsActive();
// active or still easing back to the previous view
bool IsBusy();
void Enable(const GTAped& targetPed);
// restorePreviousView = false when another camera is taking over the view
void Disable(bool restorePreviousView = true);
void SetFraming(const GTAped& targetPed, Framing framing);
void Tick();
} // namespace WardrobeCamera
} // namespace sub
