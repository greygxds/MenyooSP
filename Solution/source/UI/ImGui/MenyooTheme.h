#pragma once

#include <cstdint>

struct ImVec4;

namespace sub::Spooner::ImGuiTheme
{
	struct Color
	{
		uint8_t r = 0;
		uint8_t g = 0;
		uint8_t b = 0;
		uint8_t a = 0;

		friend bool operator==(const Color&, const Color&) = default;
	};

	struct ThemeSnapshot
	{
		Color titleBox;
		Color background;
		Color titleText;
		Color optionText;
		Color selectedText;
		Color optionBreaks;
		Color optionCount;
		Color selectionHighlight;

		friend bool operator==(const ThemeSnapshot&, const ThemeSnapshot&) = default;
	};

	ThemeSnapshot CaptureFromMenyoo();
	ImVec4 ToImGuiColor(const Color& color);
	void ApplyToImGui(const ThemeSnapshot& theme);
}
