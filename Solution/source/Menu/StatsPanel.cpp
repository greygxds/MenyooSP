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

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <dirent\include\dirent.h>

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
            [](const ColorRow&) { return lineH; },
            [](const DoubleColorRow&) { return lineH; },
            [](const ImageRow& imageRow) { return imageRow.height; }
        },
        row
    );
}

float MeasurePanelHeight(const std::vector<Row>& rows)
{
    float infoH = 0.015f;
    for (auto& row : rows)
        infoH += RowHeight(row);
    return infoH + padding * 2.0f;
}

void MenuVerticalBounds(float& menuTop, float& menuBottom)
{
    menuTop = menuPos.y + 0.0760f;
    int visibleOptions = Menu::totalOptionCount;
    if (visibleOptions > GTA_MAXOP)
        visibleOptions = GTA_MAXOP;
    menuBottom = (visibleOptions + 1.0f) * 0.035f + 0.15875f + menuPos.y;
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
    // Normal menu header top edge is 0.0760f; Draw() starts the panel background
    // one padding (0.004f) above topY, so anchor at 0.0800f for flush edges.
    return menuPos.y + 0.0800f;
}

void Draw(float topY, float width, const std::vector<Row>& rows)
{
    if (rows.empty())
        return;

    // Menu spans 0.16f +/- 0.10f: keep the legacy 0.014f/0.013f gutters so width 0.100f panels sit exactly where they used to, wider panels clear the menu.
    float panelX = 0.16f + 0.10f + 0.014f + width / 2.0f + menuPos.x;
    if (menuPos.x > 0.45f)
        panelX = 0.16f - 0.10f - 0.013f - width / 2.0f + menuPos.x;

    float panelH = MeasurePanelHeight(rows);

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
                [&](const SeparatorRow&) { DRAW_RECT(panelX, y + separatorH / 2.0f, barW, 0.002f, 80, 80, 80, 200, false); },
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
                [&](const DoubleColorRow& doubleColorRow)
                {
                    float swatchW = 0.016f;
                    float swatchH = 0.010f;
                    float leftSwatchRight = panelX - 0.002f;
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, labelColor);
                    Game::Print::drawstring(doubleColorRow.leftLabel, infoLabelX, y);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, true, false, valueColor, {0, leftSwatchRight - swatchW - 0.003f});
                    Game::Print::drawstring(doubleColorRow.leftValue, 0, y);
                    DRAW_RECT(leftSwatchRight - swatchW / 2.0f, y + lineH / 2.0f, swatchW, swatchH, doubleColorRow.leftColor.R, doubleColorRow.leftColor.G, doubleColorRow.leftColor.B, 255, false);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, labelColor);
                    Game::Print::drawstring(doubleColorRow.rightLabel, panelX + 0.002f, y);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, true, false, valueColor, {0, infoValueX - swatchW - 0.003f});
                    Game::Print::drawstring(doubleColorRow.rightValue, 0, y);
                    DRAW_RECT(infoValueX - swatchW / 2.0f, y + lineH / 2.0f, swatchW, swatchH, doubleColorRow.rightColor.R, doubleColorRow.rightColor.G, doubleColorRow.rightColor.B, 255, false);
                },
                [&](const ColorRow& colorRow)
                {
                    float swatchW = 0.016f;
                    float swatchH = 0.010f;
                    float swatchRight = infoValueX;
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, false, false, labelColor);
                    Game::Print::drawstring(colorRow.label, infoLabelX, y);
                    Game::Print::SetupDraw(0, Vector2(0.0f, 0.2f), false, true, false, valueColor, {0, swatchRight - swatchW - 0.003f});
                    Game::Print::drawstring(colorRow.valueText, 0, y);
                    DRAW_RECT(swatchRight - swatchW / 2.0f, y + lineH / 2.0f, swatchW, swatchH, colorRow.color.R, colorRow.color.G, colorRow.color.B, 255, false);
                },
                [&](const ImageRow& imageRow) { imageRow.draw(panelX, y + imageRow.height / 2.0f); }
            },
            row
        );
        y += RowHeight(row);
    }
}

void DrawAtOption(float optionTextY, float width, const std::vector<Row>& rows)
{
    if (rows.empty())
        return;

    float panelH = MeasurePanelHeight(rows);
    float menuTop = 0.0f;
    float menuBottom = 0.0f;
    MenuVerticalBounds(menuTop, menuBottom);

    float topY = MenuTopY();
    if (panelH < menuBottom - menuTop)
    {
        // Highlight center = optionhi 0.1415f - AddOption text 0.125f above the row text Y.
        float optionCenterY = optionTextY + 0.0165f;
        topY = optionCenterY - panelH / 2.0f;
        if (topY < menuTop + padding)
            topY = menuTop + padding;
        if (topY > menuBottom - panelH + padding)
            topY = menuBottom - panelH + padding;
    }

    Draw(topY, width, rows);
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

void DrawFolderContents(const std::string& dirPath, float optionTextY, float width)
{
    static std::string lastStatsDir = "";
    static std::vector<Row> statsRows;
    static const size_t maxShownEntries = 15;
    static const size_t maxEntryChars = 24;

    if (lastStatsDir != dirPath)
    {
        lastStatsDir = dirPath;
        statsRows.clear();

        std::vector<std::pair<std::string, bool>> entries;
        size_t folderCount = 0;
        size_t fileCount = 0;
        DIR* dirPoint = opendir(dirPath.c_str());
        if (dirPoint != nullptr)
        {
            dirent* entry = readdir(dirPoint);
            while (entry != nullptr)
            {
                std::string entryName = entry->d_name;
                if (!entryName.empty() && entryName.front() != '.' && entryName.front() != ',')
                {
                    bool isFolder = PathIsDirectoryA((dirPath + "\\" + entryName).c_str()) != 0;
                    entries.push_back({entryName, isFolder});
                    if (isFolder)
                        folderCount++;
                    else
                        fileCount++;
                }
                entry = readdir(dirPoint);
            }
            closedir(dirPoint);
        }

        size_t shownCount = 0;
        size_t shownFolders = 0;
        size_t shownFiles = 0;
        for (auto& entry : entries)
        {
            if (shownCount >= maxShownEntries)
                break;
            std::string shownName = entry.first;
            if (shownName.length() > maxEntryChars)
                shownName = shownName.substr(0, maxEntryChars) + "...";
            if (entry.second)
            {
                shownName += " >>>";
                shownFolders++;
            }
            else
            {
                shownFiles++;
            }
            statsRows.push_back(TextRow{shownName});
            shownCount++;
        }
        size_t hiddenFolders = folderCount - shownFolders;
        size_t hiddenFiles = fileCount - shownFiles;
        if (hiddenFolders > 0 && hiddenFiles > 0)
            statsRows.push_back(TextRow{std::to_string(hiddenFolders) + " folders and " + std::to_string(hiddenFiles) + " files more..."});
        else if (hiddenFiles > 0)
            statsRows.push_back(TextRow{std::to_string(hiddenFiles) + " files more..."});
        else if (hiddenFolders > 0)
            statsRows.push_back(TextRow{std::to_string(hiddenFolders) + " folders more..."});
        if (shownCount > 0)
            statsRows.push_back(SeparatorRow{});
        statsRows.push_back(StatRow{"Folders", std::to_string(folderCount)});
        statsRows.push_back(StatRow{"Files", std::to_string(fileCount)});
    }

    DrawAtOption(optionTextY, width, statsRows);
}

} // namespace StatsPanel
