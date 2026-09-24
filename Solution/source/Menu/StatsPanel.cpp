/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "StatsPanel.h"

#include "Menu.h" // menuPos

#include "..\Util\GTAmath.h"     // Vector2
#include "..\Natives\natives2.h" // DRAW_RECT
#include "..\Natives\types.h"    // RGBA
#include "..\Scripting\Game.h"   // Print

namespace StatsPanel
{
namespace
{
constexpr float lineH = 0.0155f;
constexpr float padding = 0.004f;
constexpr float barH = 0.018f;
constexpr float barSpacing = 0.0195f;
constexpr float separatorH = 0.007f;
constexpr float contentTop = 0.007f;

template <class... Ts> struct Overloaded : Ts...
{
    using Ts::operator()...;
};

float RowHeight(const Row& row)
{
    return std::visit(
        Overloaded{
            [](const StatRow&) { return lineH; },
            [](const DoubleStatRow&) { return lineH; },
            [](const StackedStatRow&) { return lineH * 2.0f; },
            [](const TitleRow&) { return lineH; },
            [](const TextRow&) { return lineH; },
            [](const SeparatorRow&) { return separatorH; },
            [](const SpacerRow& spacerRow) { return spacerRow.height; },
            [](const BarRow&) { return barSpacing; },
            [](const ImageRow& imageRow) { return imageRow.height; }
        },
        row
    );
}

void DrawAlignedText(const std::string& text, Align align, float leftX, float centerX, float rightX, float y, RGBA color)
{
    if (align == Align::Center)
    {
        Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), true, false, false, color);
        Game::Print::drawstring(text, centerX, y);
        return;
    }
    if (align == Align::Right)
    {
        Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, true, false, color, {0, rightX});
        Game::Print::drawstring(text, 0, y);
        return;
    }
    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, color);
    Game::Print::drawstring(text, leftX, y);
}
} // namespace

float MenuTopY()
{
    // Menu title banner is centered at 0.0989f with height 0.083f: top edge = 0.0574f.
    return menuPos.y + 0.0574f;
}

void Draw(float topY, float width, const std::vector<Row>& rows)
{
    if (rows.empty())
        return;

    float panelX = 0.324f + menuPos.x;
    if (menuPos.x > 0.45f)
        panelX = menuPos.x - 0.003f;

    float infoH = 0.015f;
    for (auto& row : rows)
        infoH += RowHeight(row);
    float panelH = infoH + padding * 2.0f;

    DRAW_RECT(panelX, topY + panelH / 2.0f - padding, width + 0.003f, panelH, 0, 0, 0, 200, false);

    float barW = width - 0.006f;
    float infoLabelX = panelX - (width / 2.0f) + 0.003f;
    float infoValueX = panelX + (width / 2.0f) - 0.003f;
    RGBA labelColor(160, 160, 160, 255);
    RGBA valueColor(255, 255, 255, 255);

    float y = topY + contentTop;
    for (auto& row : rows)
    {
        std::visit(
            Overloaded{
                [&](const StatRow& statRow)
                {
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, labelColor);
                    Game::Print::drawstring(statRow.label, infoLabelX, y);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, true, false, valueColor, {0, infoValueX});
                    Game::Print::drawstring(statRow.value, 0, y);
                },
                [&](const DoubleStatRow& doubleRow)
                {
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, labelColor);
                    Game::Print::drawstring(doubleRow.leftLabel, infoLabelX, y);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, true, false, valueColor, {0, panelX - 0.002f});
                    Game::Print::drawstring(doubleRow.leftValue, 0, y);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, labelColor);
                    Game::Print::drawstring(doubleRow.rightLabel, panelX + 0.002f, y);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, true, false, valueColor, {0, infoValueX});
                    Game::Print::drawstring(doubleRow.rightValue, 0, y);
                },
                [&](const StackedStatRow& stackedRow)
                {
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, labelColor);
                    Game::Print::drawstring(stackedRow.label, infoLabelX, y);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, valueColor);
                    Game::Print::drawstring(stackedRow.value, infoLabelX, y + lineH);
                },
                [&](const TitleRow& titleRow) { DrawAlignedText(titleRow.title, titleRow.align, infoLabelX, panelX, infoValueX, y, valueColor); },
                [&](const TextRow& textRow) { DrawAlignedText(textRow.text, textRow.align, infoLabelX, panelX, infoValueX, y, textRow.color); },
                [&](const SeparatorRow&) { DRAW_RECT(panelX, y + separatorH / 2.0f, barW, 0.001f, 80, 80, 80, 200, false); },
                [&](const SpacerRow&) {},
                [&](const BarRow& barRow)
                {
                    float ratio = barRow.ratio;
                    if (ratio < 0.0f)
                        ratio = 0.0f;
                    if (ratio > 1.0f)
                        ratio = 1.0f;

                    float barCenterY = y + barSpacing / 2.0f;
                    DRAW_RECT(panelX, barCenterY, barW, barH, 0, 0, 0, 212, false);

                    float fillWidth = barW * ratio;
                    if (fillWidth > 0.0001f)
                    {
                        float fillX = panelX - (barW / 2.0f) + (fillWidth / 2.0f);
                        DRAW_RECT(fillX, barCenterY, fillWidth, barH, 93, 182, 229, 255, false);
                    }

                    float textY = barCenterY - 0.0095f;
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.22f), false, false, true, valueColor);
                    Game::Print::drawstring(barRow.label, infoLabelX, textY);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.22f), false, true, true, valueColor, {0, infoValueX});
                    Game::Print::drawstring(barRow.valueText, 0, textY);
                },
                [&](const ImageRow& imageRow) { imageRow.draw(panelX, y + imageRow.height / 2.0f); }
            },
            row
        );
        y += RowHeight(row);
    }
}

std::vector<std::string> WrapText(const std::string& text, size_t maxChars)
{
    std::vector<std::string> wrappedLines;
    if (maxChars == 0)
    {
        wrappedLines.push_back(text);
        return wrappedLines;
    }

    std::string currentLine;
    size_t wordStart = 0;
    while (wordStart < text.length())
    {
        size_t wordEnd = text.find(' ', wordStart);
        if (wordEnd == std::string::npos)
            wordEnd = text.length();
        std::string word = text.substr(wordStart, wordEnd - wordStart);
        wordStart = wordEnd + 1;
        if (word.empty())
            continue;
        if (!currentLine.empty() && currentLine.length() + 1 + word.length() > maxChars)
        {
            wrappedLines.push_back(currentLine);
            currentLine.clear();
        }
        if (!currentLine.empty())
            currentLine += " ";
        currentLine += word;
    }
    if (!currentLine.empty())
        wrappedLines.push_back(currentLine);
    return wrappedLines;
}

} // namespace StatsPanel
