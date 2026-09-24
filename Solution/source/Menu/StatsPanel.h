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

#include "..\Natives\types.h" // RGBA

#include <functional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace StatsPanel
{
enum class Align
{
    Left,
    Center,
    Right
};

struct StatRow
{
    std::string label;
    std::string value;
};

struct DoubleStatRow
{
    std::string leftLabel;
    std::string leftValue;
    std::string rightLabel;
    std::string rightValue;
};

struct StackedStatRow
{
    std::string label;
    std::string value;
};

struct TitleRow
{
    std::string title;
    Align align = Align::Left;
};

struct TextRow
{
    std::string text;
    RGBA color = RGBA(160, 160, 160, 255);
    Align align = Align::Left;
};

struct SeparatorRow
{
};

struct SpacerRow
{
    float height = 0.008f;
};

struct BarRow
{
    std::string label;
    std::string valueText;
    float ratio = 0.0f;
};

struct ImageRow
{
    std::function<void(float centerX, float centerY)> draw;
    float width = 0.1f;
    float height = 0.0889f;
};

using Row = std::variant<StatRow, DoubleStatRow, StackedStatRow, TitleRow, TextRow, SeparatorRow, SpacerRow, BarRow, ImageRow>;

// Top edge all side panels align to: the menu title banner's top edge. Keeps panels static while hovering/scrolling.
float MenuTopY();
// topY is the panel's top edge: hover callers pass MenuTopY() for menu-top alignment.
void Draw(float topY, float width, const std::vector<Row>& rows);
std::vector<std::string> WrapText(const std::string& text, size_t maxChars);

} // namespace StatsPanel
