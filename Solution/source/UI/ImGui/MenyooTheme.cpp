#include "MenyooTheme.h"

#include "imgui.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Natives\types.h"

#include <algorithm>

namespace sub::Spooner::ImGuiTheme
{
	namespace
	{
		uint8_t Lerp(uint8_t from, uint8_t to, float amount)
		{
			return static_cast<uint8_t>(std::clamp(from + (to - from) * amount, 0.0f, 255.0f));
		}

		Color Blend(const Color& from, const Color& to, float amount)
		{
			return {
				Lerp(from.r, to.r, amount),
				Lerp(from.g, to.g, amount),
				Lerp(from.b, to.b, amount),
				Lerp(from.a, to.a, amount)
			};
		}

		Color WithAlpha(Color color, float alphaScale)
		{
			color.a = static_cast<uint8_t>(std::clamp(color.a * alphaScale, 0.0f, 255.0f));
			return color;
		}

		Color WithMinimumAlpha(Color color, uint8_t minimumAlpha)
		{
			color.a = std::max(color.a, minimumAlpha);
			return color;
		}

		Color Capture(const RGBA& color)
		{
			return {
				static_cast<uint8_t>(std::clamp(color.R, 0, 255)),
				static_cast<uint8_t>(std::clamp(color.G, 0, 255)),
				static_cast<uint8_t>(std::clamp(color.B, 0, 255)),
				static_cast<uint8_t>(std::clamp(color.A, 0, 255))
			};
		}

	}

	ImVec4 ToImGuiColor(const Color& color)
	{
		constexpr float scale = 1.0f / 255.0f;
		return ImVec4(color.r * scale, color.g * scale, color.b * scale, color.a * scale);
	}

	ThemeSnapshot CaptureFromMenyoo()
	{
		return {
			Capture(titlebox),
			Capture(BG),
			Capture(titletext),
			Capture(optiontext),
			Capture(selectedtext),
			Capture(optionbreaks),
			Capture(optioncount),
			Capture(selectionhi)
		};
	}

	void ApplyToImGui(const ThemeSnapshot& theme)
	{
		static ThemeSnapshot appliedTheme;
		static bool hasAppliedTheme = false;
		if (hasAppliedTheme && appliedTheme == theme)
			return;

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ToImGuiColor(theme.optionText);
		colors[ImGuiCol_TextDisabled] = ToImGuiColor(theme.optionBreaks);
		colors[ImGuiCol_WindowBg] = ToImGuiColor(theme.background);
		colors[ImGuiCol_ChildBg] = ToImGuiColor(theme.background);
		colors[ImGuiCol_PopupBg] = ToImGuiColor(WithMinimumAlpha(theme.background, 230));
		colors[ImGuiCol_Border] = ToImGuiColor(Blend(theme.background, theme.optionBreaks, 0.60f));
		colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_FrameBg] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.25f));
		colors[ImGuiCol_FrameBgHovered] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.55f));
		colors[ImGuiCol_FrameBgActive] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.80f));
		colors[ImGuiCol_TitleBg] = ToImGuiColor(theme.titleBox);
		colors[ImGuiCol_TitleBgActive] = ToImGuiColor(theme.titleBox);
		colors[ImGuiCol_TitleBgCollapsed] = ToImGuiColor(theme.background);
		colors[ImGuiCol_MenuBarBg] = ToImGuiColor(theme.titleBox);
		colors[ImGuiCol_ScrollbarBg] = ToImGuiColor(WithAlpha(theme.background, 0.70f));
		colors[ImGuiCol_Header] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.45f));
		colors[ImGuiCol_HeaderHovered] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.70f));
		colors[ImGuiCol_HeaderActive] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 1.00f));
		colors[ImGuiCol_Button] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.25f));
		colors[ImGuiCol_ButtonHovered] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.55f));
		colors[ImGuiCol_ButtonActive] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.80f));
		colors[ImGuiCol_Separator] = ToImGuiColor(theme.optionBreaks);
		colors[ImGuiCol_SeparatorHovered] = ToImGuiColor(Blend(theme.optionBreaks, theme.selectionHighlight, 0.50f));
		colors[ImGuiCol_SeparatorActive] = ToImGuiColor(theme.selectionHighlight);
		colors[ImGuiCol_CheckMark] = ToImGuiColor(theme.selectionHighlight);
		colors[ImGuiCol_SliderGrab] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.70f));
		colors[ImGuiCol_SliderGrabActive] = ToImGuiColor(theme.selectionHighlight);
		colors[ImGuiCol_ScrollbarGrab] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.45f));
		colors[ImGuiCol_ScrollbarGrabHovered] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 0.70f));
		colors[ImGuiCol_ScrollbarGrabActive] = ToImGuiColor(Blend(theme.background, theme.selectionHighlight, 1.00f));
		colors[ImGuiCol_TextSelectedBg] = ToImGuiColor(WithAlpha(theme.selectionHighlight, 0.40f));
		colors[ImGuiCol_NavHighlight] = ToImGuiColor(WithAlpha(theme.selectionHighlight, 0.80f));

		appliedTheme = theme;
		hasAppliedTheme = true;
	}
}
