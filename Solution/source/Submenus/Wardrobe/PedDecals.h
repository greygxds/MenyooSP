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

#include "..\..\Scripting\GTAped.h"

#include <string>

namespace sub
{
namespace PedDecals
{
void LoadDecalCatalog();
std::string FindDecalCaption(Hash collectionHash, Hash overlayHash);
void OpenDecalsMenu();
} // namespace PedDecals
} // namespace sub
