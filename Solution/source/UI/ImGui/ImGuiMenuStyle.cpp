#include "ImGuiMenuStyle.h"

#include "imgui.h"

namespace sub::Spooner::ImGuiMenuStyle
{
	ScopedPopupStyle::ScopedPopupStyle(const PopupConfig& config)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(13.0f * config.scale, 14.0f * config.scale));
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 10.0f * config.scale);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 7.0f * config.scale);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 5.0f * config.scale);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, config.background);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, config.scrollbarGrab);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, config.scrollbarGrabHovered);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, config.scrollbarGrabActive);
	}

	ScopedPopupStyle::~ScopedPopupStyle()
	{
		ImGui::PopStyleColor(5);
		ImGui::PopStyleVar(5);
	}

	ScopedRowStyle::ScopedRowStyle(const ImGuiTheme::ThemeSnapshot& theme)
	{
		const ImVec4 hover = ImGuiTheme::ToImGuiColor(theme.selectionHighlight);
		const Metrics metrics = Metrics::FromFontSize(ImGui::GetFontSize());
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(hover.x, hover.y, hover.z, 0.16f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(hover.x, hover.y, hover.z, 0.42f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(hover.x, hover.y, hover.z, 0.58f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(metrics.framePaddingX, metrics.framePaddingY));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(metrics.itemSpacingX, metrics.itemSpacingY));
	}

	ScopedRowStyle::~ScopedRowStyle()
	{
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
	}
}
