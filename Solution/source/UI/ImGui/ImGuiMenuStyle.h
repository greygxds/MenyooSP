#pragma once

#include "imgui.h"
#include "MenyooTheme.h"

namespace sub::Spooner::ImGuiMenuStyle
{
	struct FontSizes
	{
		static constexpr float Main = 20.0f;
		static constexpr float Small = 18.0f;
		static constexpr float MenuBar = 24.0f;
		static constexpr float Icon = 15.0f;
		static constexpr float HeaderIcon = 30.0f;
	};

	struct PopupConfig
	{
		ImVec4 background;
		ImVec4 scrollbarGrab;
		ImVec4 scrollbarGrabHovered;
		ImVec4 scrollbarGrabActive;
		float scale = 1.0f;
	};

	class ScopedPopupStyle
	{
	public:
		explicit ScopedPopupStyle(const PopupConfig& config);
		~ScopedPopupStyle();
		ScopedPopupStyle(const ScopedPopupStyle&) = delete;
		ScopedPopupStyle& operator=(const ScopedPopupStyle&) = delete;
	};

	struct Metrics
	{
		float scale;
		float framePaddingX;
		float framePaddingY;
		float itemSpacingX;
		float itemSpacingY;
		float rowRounding;

		static Metrics FromFontSize(float fontSize)
		{
			const float scale = fontSize / 16.0f;
			return { scale, 7.0f * scale, 4.0f * scale, 7.0f * scale, 8.0f * scale, 7.0f * scale };
		}
	};

	class ScopedRowStyle
	{
	public:
		explicit ScopedRowStyle(const ImGuiTheme::ThemeSnapshot& theme);
		~ScopedRowStyle();
		ScopedRowStyle(const ScopedRowStyle&) = delete;
		ScopedRowStyle& operator=(const ScopedRowStyle&) = delete;
	};
}
