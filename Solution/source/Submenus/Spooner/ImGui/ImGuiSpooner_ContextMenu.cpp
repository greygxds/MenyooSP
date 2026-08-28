#include "ImGuiSpooner.h"
#include "imgui.h"
#include "..\SpoonerSettings.h"
#include "..\..\..\Scripting\World.h"
#include "..\..\..\Menu\Menu.h"
#include "..\..\..\Menu\submenu_enum.h"
#include "..\..\..\UI\ImGui\ImGuiMenuStyle.h"

#include <cctype>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <vector>

namespace sub::Spooner::ImGuiSpooner
{
	struct DragSelectionState
	{
		bool active = false;
		bool dragging = false;
		ImVec2 start{};
		ImVec2 current{};
		bool additive = false;
	};

	static DragSelectionState g_dragSelection;

	static void DrawDragSelection(const ImGuiIO& io)
	{
		if (!g_dragSelection.dragging)
			return;

		const ImVec2 minPoint(
			(std::min)(g_dragSelection.start.x, g_dragSelection.current.x),
			(std::min)(g_dragSelection.start.y, g_dragSelection.current.y));
		const ImVec2 maxPoint(
			(std::max)(g_dragSelection.start.x, g_dragSelection.current.x),
			(std::max)(g_dragSelection.start.y, g_dragSelection.current.y));
		const ImVec4 fill = ImGuiTheme::ToImGuiColor(g_Shared.theme.selectionHighlight);
		const ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(fill.x, fill.y, fill.z, 0.18f));
		const ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(fill.x, fill.y, fill.z, 0.90f));
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		drawList->AddRectFilled(minPoint, maxPoint, fillColor, 2.0f);
		drawList->AddRect(minPoint, maxPoint, borderColor, 2.0f, 0, 1.5f);
		(void)io;
	}

// ── Fuzzy matching helpers ────────────────────────────────────────
static bool Match_Substring(const char* str, const char* query)
{
	if (!*query) return true;
	for (; *str; ++str)
	{
		const char* s = str;
		const char* q = query;
		while (*s && *q && std::tolower(static_cast<unsigned char>(*s)) == std::tolower(static_cast<unsigned char>(*q)))
		{
			++s; ++q;
		}
		if (!*q) return true;
	}
	return false;
}

static bool Match_Subsequence(const char* str, const char* query)
{
	if (!*query) return true;
	for (; *str; ++str)
	{
		if (std::tolower(static_cast<unsigned char>(*str)) == std::tolower(static_cast<unsigned char>(*query)))
		{
			++query;
			if (!*query) return true;
		}
	}
	return false;
}

static int Match_Levenshtein(const char* s, const char* t)
{
	int n = static_cast<int>(std::strlen(s));
	int m = static_cast<int>(std::strlen(t));
	if (n == 0) return m;
	if (m == 0) return n;

	std::vector<int> prev(static_cast<size_t>(n) + 1);
	std::vector<int> curr(static_cast<size_t>(n) + 1);
	for (int i = 0; i <= n; ++i) prev[static_cast<size_t>(i)] = i;

	for (int j = 1; j <= m; ++j)
	{
		curr[0] = j;
		char t_j = static_cast<char>(std::tolower(static_cast<unsigned char>(t[j - 1])));
		for (int i = 1; i <= n; ++i)
		{
			char s_i = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i - 1])));
			int cost = (s_i == t_j) ? 0 : 1;
			int a = curr[static_cast<size_t>(i - 1)] + 1;
			int b = prev[static_cast<size_t>(i)] + 1;
			int c = prev[static_cast<size_t>(i - 1)] + cost;
			curr[static_cast<size_t>(i)] = (std::min)(a, (std::min)(b, c));
		}
		prev.swap(curr);
	}
	return prev[static_cast<size_t>(n)];
}

int Match_Score(const char* label, const char* query)
{
	if (Match_Substring(label, query)) return 0;
	if (Match_Subsequence(label, query)) return 1;

	int dist = Match_Levenshtein(label, query);
	int qlen = static_cast<int>(std::strlen(query));
	int threshold = (std::max)(2, qlen / 3);
	if (dist <= threshold) return dist + 1;

	return -1;
}

// ── Context menu ──────────────────────────────────────────────────
void HandleCursorModeClicks(ImGuiIO& io)
{
	if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
		return;

	// Ignore clicks if the gizmo is being used
	if (g_Shared.render.gizmoUsing)
		return;

	if (!g_dragSelection.active && io.MouseClicked[0] && !io.WantCaptureMouse)
	{
		g_dragSelection.active = true;
		g_dragSelection.dragging = false;
		g_dragSelection.start = io.MousePos;
		g_dragSelection.current = io.MousePos;
		g_dragSelection.additive = io.KeyShift;
	}

	if (g_dragSelection.active)
	{
		g_dragSelection.current = io.MousePos;
		if (io.MouseDown[0])
		{
			const float dx = g_dragSelection.current.x - g_dragSelection.start.x;
			const float dy = g_dragSelection.current.y - g_dragSelection.start.y;
			g_dragSelection.dragging = g_dragSelection.dragging || (dx * dx + dy * dy >= 49.0f);
			DrawDragSelection(io);
			return;
		}

		if (g_dragSelection.dragging)
		{
			const ImVec2 minPoint(
				(std::min)(g_dragSelection.start.x, g_dragSelection.current.x),
				(std::min)(g_dragSelection.start.y, g_dragSelection.current.y));
			const ImVec2 maxPoint(
				(std::max)(g_dragSelection.start.x, g_dragSelection.current.x),
				(std::max)(g_dragSelection.start.y, g_dragSelection.current.y));
			SelectionRectangle selection{
				minPoint.x / io.DisplaySize.x,
				minPoint.y / io.DisplaySize.y,
				maxPoint.x / io.DisplaySize.x,
				maxPoint.y / io.DisplaySize.y,
				g_dragSelection.additive};
			SetCommand(g_Shared, CursorCommand::SelectEntitiesInRectangle, 0, -1, 0.0f, {}, selection);
		}
		else
		{
			g_Shared.cursorScreenX = (g_dragSelection.start.x / io.DisplaySize.x) * 2.0f - 1.0f;
			g_Shared.cursorScreenY = (g_dragSelection.start.y / io.DisplaySize.y) * 2.0f - 1.0f;
			SetCommand(g_Shared, CursorCommand::SelectEntity);
		}

		g_dragSelection = DragSelectionState{};
		return;
	}

	// Handle left click (select entity), ignore left clicks in ImGui windows
	// Handle right click (select entity and show context menu)
	if (io.MouseClicked[1] && !io.WantCaptureMouse)
	{
		g_Shared.cursorScreenX = (io.MousePos.x / io.DisplaySize.x) * 2.0f - 1.0f;
		g_Shared.cursorScreenY = (io.MousePos.y / io.DisplaySize.y) * 2.0f - 1.0f;
		SetCommand(g_Shared, CursorCommand::SelectEntityAndShowMenu);
	}
}

void CancelDragSelection()
{
	g_dragSelection = DragSelectionState{};
}

// ── Context menu search ───────────────────────────────────────────
enum class ContextIcon : uint8_t
{
	None,
	Search,
	Edit,
	ManualEdit,
	PersonRunning,
	Link,
	Task,
	Timeline,
	Box,
	PlaceGround,
	Select,
	Expand,
	HardDrive,
	Bookmark,
	Database,
	List,
	Unlink,
	Shirt,
	Car,
	Wrench,
	Flag,
	Copy,
	Trash,
	Chevron,
	Check
};

struct SearchHit
{
	const char* label;
	std::string searchText;
	char breadcrumb[64];
	CursorCommand command;
	ContextIcon icon;
	bool isToggle;
	bool toggleState;
};

thread_local std::vector<SearchHit> g_searchPool;
thread_local std::vector<const char*> g_breadcrumbStack;
thread_local bool g_searchBuildMode = false;

static char g_ctxSearchBuf[64] = "";

static const char* EntityTypeLabel(int type)
{
	return type == 1 ? "Ped" : type == 2 ? "Vehicle" : type == 3 ? "Prop" : "Entity";
}

static const char* IconGlyph(ContextIcon icon)
{
	switch (icon)
	{
	case ContextIcon::Search: return "\xEF\x80\x82";
	case ContextIcon::Edit: return "\xEF\x81\x84";
	case ContextIcon::ManualEdit: return "\xEF\x82\xB2"; // up-down-left-right
	case ContextIcon::PersonRunning: return "\xEF\x9C\x8C"; // person-running
	case ContextIcon::Link: return "\xEF\x83\x81";
	case ContextIcon::Task: return "\xEF\x87\x98";
	case ContextIcon::Timeline: return "\xEE\x8A\x9C"; // timeline
	case ContextIcon::Box: return "\xEF\x91\xA6";
	case ContextIcon::PlaceGround: return "\xEE\x92\xB8"; // arrows-down-to-line
	case ContextIcon::Select: return "\xEF\x89\x85";
	case ContextIcon::Expand: return "\xEF\x81\xA5";
	case ContextIcon::HardDrive: return "\xEF\x82\xA0"; // hard-drive
	case ContextIcon::Bookmark: return "\xEF\x80\xAE"; // bookmark
	case ContextIcon::Database: return "\xEF\x87\x80";
	case ContextIcon::List: return "\xEF\x80\xBA";
	case ContextIcon::Unlink: return "\xEF\x84\xA7";
	case ContextIcon::Shirt: return "\xEF\x95\x93";
	case ContextIcon::Car: return "\xEF\x97\xA4";
	case ContextIcon::Wrench: return "\xEF\x82\xAD";
	case ContextIcon::Flag: return "\xEF\x80\xA4";
	case ContextIcon::Copy: return "\xEF\x83\x85";
	case ContextIcon::Trash: return "\xEF\x8B\xAD";
	case ContextIcon::Chevron: return "\xEF\x84\x85";
	case ContextIcon::Check: return "\xEF\x80\x8C";
	default: return "";
	}
}

static ImVec4 BlendImGuiColor(const ImVec4& from, const ImVec4& to, float amount)
{
	return ImVec4(from.x + (to.x - from.x) * amount, from.y + (to.y - from.y) * amount,
		from.z + (to.z - from.z) * amount, from.w + (to.w - from.w) * amount);
}

static ImVec4 WithImGuiAlpha(ImVec4 color, float alpha)
{
	color.w = alpha;
	return color;
}

static void DrawIconCentered(ImDrawList* drawList, ImFont* font, ContextIcon icon,
	const ImVec2& cellMin, const ImVec2& cellSize, const ImVec4& color)
{
	if (!font || icon == ContextIcon::None)
		return;

	const char* glyphText = IconGlyph(icon);
	const auto* utf8 = reinterpret_cast<const unsigned char*>(glyphText);
	const unsigned int codepoint = ((utf8[0] & 0x0f) << 12) | ((utf8[1] & 0x3f) << 6) | (utf8[2] & 0x3f);
	const ImFontGlyph* glyph = font->FindGlyph(static_cast<ImWchar>(codepoint));
	ImVec2 glyphPos(cellMin.x, cellMin.y + (cellSize.y - font->FontSize) * 0.5f);
	if (glyph)
	{
		glyphPos.x += (cellSize.x - (glyph->X1 - glyph->X0)) * 0.5f - glyph->X0;
		glyphPos.y = cellMin.y + (cellSize.y - (glyph->Y1 - glyph->Y0)) * 0.5f - glyph->Y0;
	}
	drawList->AddText(font, font->FontSize, glyphPos, ImGui::ColorConvertFloat4ToU32(color), glyphText);
}

static void DrawMenuIcon(ContextIcon icon, bool destructiveHover = false)
{
	const float scale = ImGui::GetFontSize() / 16.0f;
	const float iconColumnWidth = 34.0f * scale;
	const float iconCellWidth = 18.0f * scale;
	const float iconInset = 8.0f * scale;
	const float rowStart = ImGui::GetCursorPosX();
	if (g_IconFont && icon != ContextIcon::None)
	{
		const ImVec2 screenPos = ImGui::GetCursorScreenPos();
		const ImVec4 iconColor = destructiveHover
			? ImVec4(0.90f, 0.38f, 0.38f, 0.90f)
			: WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.optionText), 0.58f);
		DrawIconCentered(ImGui::GetWindowDrawList(), g_IconFont, icon,
			ImVec2(screenPos.x + iconInset, screenPos.y), ImVec2(iconCellWidth, ImGui::GetTextLineHeight()), iconColor);
	}
	ImGui::SetCursorPosX(rowStart + iconColumnWidth);
}

static bool DrawRoundedRowBackground(bool destructive = false)
{
	const float scale = ImGui::GetFontSize() / 16.0f;
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const ImVec2 rowPos = ImGui::GetCursorScreenPos();
	const ImVec2 rowMin(windowPos.x + ImGui::GetWindowContentRegionMin().x + 2.0f * scale, rowPos.y - 3.0f * scale);
	const ImVec2 rowMax(windowPos.x + ImGui::GetWindowContentRegionMax().x - 1.0f * scale,
		rowPos.y + ImGui::GetTextLineHeight() + 3.0f * scale);
	const bool hovered = ImGui::IsMouseHoveringRect(rowMin, rowMax);
	if (hovered)
	{
		const ImVec4 color = destructive
			? ImVec4(0.55f, 0.12f, 0.12f, ImGui::IsMouseDown(ImGuiMouseButton_Left) ? 0.42f : 0.28f)
			: WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.selectionHighlight),
				ImGui::IsMouseDown(ImGuiMouseButton_Left) ? 0.42f : 0.28f);
		ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, ImGui::ColorConvertFloat4ToU32(color), 5.0f * scale);
	}
	return hovered;
}

static void DrawRightAccessory(ImDrawList* drawList, ContextIcon icon, const ImVec2& rowPos,
	float rowRight, float rowHeight, const ImVec4& color)
{
	const float scale = ImGui::GetFontSize() / 16.0f;
	const ImVec2 clipMin(rowRight - 24.0f * scale, rowPos.y - 3.0f * scale);
	const ImVec2 clipMax(rowRight, rowPos.y + rowHeight + 3.0f * scale);
	drawList->PushClipRect(clipMin, clipMax, true);
	DrawIconCentered(drawList, g_IconFont, icon, ImVec2(rowRight - 20.0f * scale, clipMin.y),
		ImVec2(12.0f * scale, clipMax.y - clipMin.y), color);
	drawList->PopClipRect();
}

static bool DrawStyledMenuItem(const char* label, const char* shortcut, bool selected, ContextIcon icon,
	bool destructive = false)
{
	const ImVec2 rowPos = ImGui::GetCursorScreenPos();
	const float rowHeight = ImGui::GetTextLineHeight();
	const float rowRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const bool hovered = DrawRoundedRowBackground(destructive);
	if (hovered)
		ImGui::PushStyleColor(ImGuiCol_Text, destructive
			? ImVec4(0.94f, 0.54f, 0.54f, 1.0f)
			: ImGuiTheme::ToImGuiColor(g_Shared.theme.optionText));
	DrawMenuIcon(icon, destructive && hovered);
	const bool drawBreadcrumb = shortcut && shortcut[0] && g_SmallFont;
	const bool activated = ImGui::MenuItem(label, drawBreadcrumb ? nullptr : shortcut, false);
	if (hovered)
		ImGui::PopStyleColor();
	if (drawBreadcrumb)
	{
		ImGui::PushFont(g_SmallFont);
		const ImVec2 breadcrumbSize = ImGui::CalcTextSize(shortcut);
		ImGui::PopFont();
		const float scale = ImGui::GetFontSize() / 16.0f;
		const float rightInset = (selected ? 28.0f : 8.0f) * scale;
		const ImVec2 breadcrumbPos(rowRight - rightInset - breadcrumbSize.x,
			rowPos.y + (rowHeight - g_SmallFont->FontSize) * 0.5f);
		const ImVec4 breadcrumbColor = WithImGuiAlpha(
			ImGuiTheme::ToImGuiColor(g_Shared.theme.optionBreaks), 0.72f);
		drawList->PushClipRect(ImVec2(rowRight - (breadcrumbSize.x + rightInset), rowPos.y - 2.0f * scale),
			ImVec2(rowRight - (selected ? 24.0f : 0.0f) * scale, rowPos.y + rowHeight + 2.0f * scale), true);
		drawList->AddText(g_SmallFont, g_SmallFont->FontSize, breadcrumbPos,
			ImGui::ColorConvertFloat4ToU32(breadcrumbColor), shortcut);
		drawList->PopClipRect();
	}
	if (selected)
	{
		const ImVec4 checkColor = WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.optionText), hovered ? 0.88f : 0.58f);
		DrawRightAccessory(drawList, ContextIcon::Check, rowPos, rowRight, rowHeight, checkColor);
	}
	return activated;
}

static bool CtxBeginMenu(const char* label, ContextIcon icon = ContextIcon::None)
{
	if (g_searchBuildMode)
	{
		g_breadcrumbStack.push_back(label);
		return true;
	}
	const float scale = ImGui::GetFontSize() / 16.0f;
	const ImVec2 rowPos = ImGui::GetCursorScreenPos();
	const float rowHeight = ImGui::GetTextLineHeight();
	const float rowRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	ImDrawList* parentDrawList = ImGui::GetWindowDrawList();
	const bool hovered = DrawRoundedRowBackground();
	if (hovered)
		ImGui::PushStyleColor(ImGuiCol_Text, ImGuiTheme::ToImGuiColor(g_Shared.theme.optionText));
	DrawMenuIcon(icon);
	parentDrawList->PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(rowRight - 24.0f * scale, FLT_MAX), true);
	ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f),
		ImVec2(FLT_MAX, ImGui::GetIO().DisplaySize.y * 0.80f));
	const bool open = ImGui::BeginMenu(label);
	parentDrawList->PopClipRect();
	if (hovered)
		ImGui::PopStyleColor();
	const ImVec4 arrowColor = WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.optionText),
		(hovered || open) ? 0.82f : 0.42f);
	DrawRightAccessory(parentDrawList, ContextIcon::Chevron, rowPos, rowRight, rowHeight, arrowColor);
	if (open)
		g_breadcrumbStack.push_back(label);
	return open;
}

static void CtxEndMenu()
{
	if (!g_searchBuildMode) ImGui::EndMenu();
	g_breadcrumbStack.pop_back();
}

static void CtxSeparator()
{
	if (!g_searchBuildMode)
	{
		const float padding = 2.0f * (ImGui::GetFontSize() / 16.0f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding);
		const float startX = ImGui::GetCursorPosX() + 34.0f * (ImGui::GetFontSize() / 16.0f);
		const float endX = ImGui::GetWindowContentRegionMax().x;
		const float screenY = ImGui::GetCursorScreenPos().y;
		const ImVec2 windowPos = ImGui::GetWindowPos();
		ImGui::GetWindowDrawList()->AddLine(ImVec2(windowPos.x + startX, screenY),
			ImVec2(windowPos.x + endX, screenY), ImGui::GetColorU32(ImGuiCol_Separator));
		ImGui::Dummy(ImVec2(0.0f, 1.0f));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding);
	}
}

static void CtxGroupSpacing()
{
	if (!g_searchBuildMode)
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f * (ImGui::GetFontSize() / 16.0f));
}

static bool CtxMenuItem(const char* label, CursorCommand cmd, ContextIcon icon = ContextIcon::None,
	bool isToggle = false, bool toggleState = false, std::initializer_list<const char*> aliases = {}, bool destructive = false)
{
	SearchHit hit;
	hit.label = label;
	hit.command = cmd;
	hit.icon = icon;
	hit.isToggle = isToggle;
	hit.toggleState = toggleState;
	hit.searchText = label;
	for (auto& alias : aliases)
	{
		hit.searchText += ' ';
		hit.searchText += alias;
	}
	hit.breadcrumb[0] = '\0';
	for (size_t i = 0; i < g_breadcrumbStack.size(); i++)
	{
		if (i > 0) strcat_s(hit.breadcrumb, sizeof(hit.breadcrumb), " → ");
		strcat_s(hit.breadcrumb, sizeof(hit.breadcrumb), g_breadcrumbStack[i]);
	}
	g_searchPool.push_back(hit);

	if (g_searchBuildMode) return false;
	return DrawStyledMenuItem(label, nullptr, isToggle ? toggleState : false, icon, destructive);
}

static void DrawEmptySpaceMenu()
{
	ImGui::SeparatorText("Empty Space");

	if (CtxBeginMenu("Spawn", ContextIcon::Box))
	{
		const auto drawFavourites = [](const char* label, const std::vector<FavouriteEntry>& entries, uint8_t category)
		{
			if (!CtxBeginMenu(label))
				return;
			for (const auto& entry : entries)
			{
				if (ImGui::MenuItem(entry.name.c_str()))
					SetCommand(g_Shared, CursorCommand::SpawnFavourite, 0, -1, 0.0f,
						FavouriteSpawnPayload{ category, entry.modelHash, entry.name });
			}
			CtxEndMenu();
		};

		drawFavourites("Props", g_Shared.favouriteCache.props, 0);
		drawFavourites("Peds", g_Shared.favouriteCache.peds, 1);
		drawFavourites("Vehicles", g_Shared.favouriteCache.vehicles, 2);
		CtxEndMenu();
	}

	if (!g_Shared.dbEntityCache.empty())
	{
		if (CtxBeginMenu("Place Entity Here", ContextIcon::Box))
		{
			for (auto& entry : g_Shared.dbEntityCache)
			{
				ImGui::PushID(entry.entityHandle);
				if (ImGui::MenuItem(entry.hashName.c_str()))
					SetCommand(g_Shared, CursorCommand::EmptyMenu_PlaceEntityHere, 0, entry.entityHandle);
				ImGui::PopID();
			}
			CtxEndMenu();
		}
	}
	else
	{
		ImGui::TextDisabled("No entities in database");
	}
}

// Drawn only when the search box is active and has text
static void DrawContextMenu_SearchMode()
{
	struct Match { int score; int index; };
	std::vector<Match> matches;

	for (int i = 0; i < (int)g_searchPool.size(); i++)
	{
		auto& hit = g_searchPool[i];
			int score = Match_Score(hit.searchText.c_str(), g_ctxSearchBuf);
		if (score >= 0)
			matches.push_back({ score, i });
	}

	if (matches.empty())
	{
		ImGui::TextDisabled("No matches");
	}
	else
	{
		std::sort(matches.begin(), matches.end(),
			[](const Match& a, const Match& b) { return a.score < b.score; });

		for (auto& m : matches)
		{
			auto& hit = g_searchPool[m.index];
			const char* bc = hit.breadcrumb[0] ? hit.breadcrumb : nullptr;
			if (DrawStyledMenuItem(hit.label, bc, hit.isToggle ? hit.toggleState : false, hit.icon))
				SetCommand(g_Shared, hit.command);
		}
	}
}

static void DrawVehicleWindowsImGui()
{
	static constexpr const char* names[] = { "Front Left", "Front Right", "Rear Left", "Rear Right" };
	static constexpr const char* actions[] = { "Open", "Close", "Break", "Fix", "Remove" };
	for (int w = 0; w < 4; ++w)
	{
		if (!CtxBeginMenu(names[w])) continue;
		for (int a = 0; a < 5; ++a)
			if (CtxMenuItem(actions[a], CursorCommand::RmbMenu_WindowAction)) SetCommand(g_Shared, CursorCommand::RmbMenu_WindowAction, a * 10 + w);
		CtxEndMenu();
	}
	if (CtxBeginMenu("All Windows"))
	{
		for (int a = 0; a < 5; ++a)
			if (CtxMenuItem(actions[a], CursorCommand::RmbMenu_WindowAction)) SetCommand(g_Shared, CursorCommand::RmbMenu_WindowAction, a * 10 + 4);
		CtxEndMenu();
	}
}

static void DrawVehicleDoorsImGui()
{
	static constexpr const char* names[] = { "Driver Door", "Passenger Door", "Rear Left", "Rear Right", "Hood", "Trunk" };
	for (int d = 0; d < 6; ++d)
	{
		if (!CtxBeginMenu(names[d])) continue;
		const int action = g_Shared.cache.vehicleDoorOpen[d] ? 1 : 2;
		const char* label = g_Shared.cache.vehicleDoorOpen[d] ? "Close" : "Open";
		if (CtxMenuItem(label, CursorCommand::RmbMenu_DoorAction))
			SetCommand(g_Shared, CursorCommand::RmbMenu_DoorAction, action * 10 + d);
		if (CtxMenuItem("Break", CursorCommand::RmbMenu_DoorAction)) SetCommand(g_Shared, CursorCommand::RmbMenu_DoorAction, 3 * 10 + d);
		if (CtxMenuItem("Fix", CursorCommand::RmbMenu_DoorAction)) SetCommand(g_Shared, CursorCommand::RmbMenu_DoorAction, 4 * 10 + d);
		CtxEndMenu();
	}
	if (CtxBeginMenu("All Doors"))
	{
		const bool anyOpen = std::any_of(std::begin(g_Shared.cache.vehicleDoorOpen), std::end(g_Shared.cache.vehicleDoorOpen), [](bool open) { return open; });
		const int action = anyOpen ? 1 : 2;
		if (CtxMenuItem(anyOpen ? "Close" : "Open", CursorCommand::RmbMenu_DoorAction))
			SetCommand(g_Shared, CursorCommand::RmbMenu_DoorAction, action * 10 + 7);
		if (CtxMenuItem("Break", CursorCommand::RmbMenu_DoorAction)) SetCommand(g_Shared, CursorCommand::RmbMenu_DoorAction, 3 * 10 + 7);
		if (CtxMenuItem("Fix", CursorCommand::RmbMenu_DoorAction)) SetCommand(g_Shared, CursorCommand::RmbMenu_DoorAction, 4 * 10 + 7);
		CtxEndMenu();
	}
}

static void DrawVehicleHealthImGui()
{
	static constexpr float values[] = { 1000.0f, 750.0f, 500.0f, 250.0f, 100.0f };
	static constexpr const char* labels[] = { "1000", "750", "500", "250", "100" };
	if (CtxBeginMenu("Body Health"))
	{
		for (int i = 0; i < 5; ++i) if (CtxMenuItem(labels[i], CursorCommand::RmbMenu_HealthSet)) SetCommand(g_Shared, CursorCommand::RmbMenu_HealthSet, 0, -1, values[i]);
		CtxEndMenu();
	}
	if (CtxBeginMenu("Engine Health"))
	{
		for (int i = 0; i < 5; ++i) if (CtxMenuItem(labels[i], CursorCommand::RmbMenu_HealthSet)) SetCommand(g_Shared, CursorCommand::RmbMenu_HealthSet, 1, -1, values[i]);
		CtxEndMenu();
	}
	if (CtxBeginMenu("Petrol Tank Health"))
	{
		for (int i = 0; i < 5; ++i) if (CtxMenuItem(labels[i], CursorCommand::RmbMenu_HealthSet)) SetCommand(g_Shared, CursorCommand::RmbMenu_HealthSet, 2, -1, values[i]);
		CtxEndMenu();
	}
	if (CtxMenuItem("Repair Vehicle", CursorCommand::RmbMenu_Repair)) SetCommand(g_Shared, CursorCommand::RmbMenu_Repair);
}

static void DrawVehicleExtrasImGui()
{
	for (int id = 0; id <= 60; ++id)
	{
		const std::string label = "Extra " + std::to_string(id);
		if (CtxMenuItem(label.c_str(), CursorCommand::RmbMenu_ExtraToggle, ContextIcon::None, false, false, { "Toggle" }))
			SetCommand(g_Shared, CursorCommand::RmbMenu_ExtraToggle, id);
	}
}

static void DrawVehicleNeonsImGui()
{
	static constexpr const char* names[] = { "Left Neon", "Right Neon", "Front Neon", "Back Neon" };
	for (int i = 0; i < 4; ++i)
		if (CtxMenuItem(names[i], CursorCommand::RmbMenu_NeonToggle, ContextIcon::None, false, false, { "Toggle", "Neon" }))
			SetCommand(g_Shared, CursorCommand::RmbMenu_NeonToggle, i);
}

static void DrawVehicleLightsImGui()
{
	static constexpr const char* names[] = { "Headlights", "Left Indicator", "Right Indicator", "Hazard Lights" };
	for (int i = 0; i < 4; ++i)
		if (CtxMenuItem(names[i], CursorCommand::RmbMenu_LightToggle, ContextIcon::None, false, false, { "Toggle", "Lights" }))
			SetCommand(g_Shared, CursorCommand::RmbMenu_LightToggle, i);
}

static void DrawContextMenu_Normal()
{
	if (CtxMenuItem("In Database", CursorCommand::RmbMenu_DbToggle, ContextIcon::HardDrive, true,
		g_Shared.cache.entityInDb, { "Database", "Add", "Remove", "Save" }))
		SetCommand(g_Shared, CursorCommand::RmbMenu_DbToggle);
	const char* favouriteLabel = g_Shared.cache.entityFavourite ? "Remove from Favourites" : "Add to Favourites";
	if (CtxMenuItem(favouriteLabel, CursorCommand::RmbMenu_FavouriteToggle, ContextIcon::Bookmark,
		false, false, { "Favourite", "Favorite" }, g_Shared.cache.entityFavourite))
		SetCommand(g_Shared, CursorCommand::RmbMenu_FavouriteToggle);
	CtxSeparator();

	if (CtxBeginMenu("Entity Flags", ContextIcon::Flag))
	{
		if (CtxMenuItem("Frozen In Place", CursorCommand::RmbMenu_Frozen, ContextIcon::None, true, g_Shared.cache.entityFrozen, { "Freeze", "Lock" }))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Frozen);
		if (CtxMenuItem("Collision", CursorCommand::RmbMenu_Collision, ContextIcon::None, true, g_Shared.cache.entityCollision, { "Physics" }))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Collision);
		CtxEndMenu();
	}
	CtxSeparator();

	if (CtxMenuItem("Place On Ground", CursorCommand::RmbMenu_PlaceOnGround, ContextIcon::PlaceGround))
		SetCommand(g_Shared, CursorCommand::RmbMenu_PlaceOnGround);
	if (CtxBeginMenu("Select Entities In Radius", ContextIcon::Expand))
	{
		constexpr float radii[] = { 5.0f, 10.0f, 25.0f, 50.0f };
		constexpr const char* labels[] = {
			"5 m",
			"10 m",
			"25 m",
			"50 m" };
		for (size_t i = 0; i < std::size(radii); ++i)
		{
			if (CtxMenuItem(labels[i], CursorCommand::RmbMenu_SelectRadius, ContextIcon::None, false, false,
				{ "Multi-Select", "Radius" }))
				SetCommand(g_Shared, CursorCommand::RmbMenu_SelectRadius, 0, -1, radii[i]);
		}
		CtxEndMenu();
	}
	CtxSeparator();

	if (CtxMenuItem("Manual Editing", CursorCommand::RmbMenu_ManualEditing, ContextIcon::ManualEdit))
		SetCommand(g_Shared, CursorCommand::RmbMenu_ManualEditing);
	if (CtxMenuItem("Attachment Options", CursorCommand::RmbMenu_Attachment, ContextIcon::Link))
		SetCommand(g_Shared, CursorCommand::RmbMenu_Attachment);
	if (CtxMenuItem("Task Sequence", CursorCommand::RmbMenu_TaskSequence, ContextIcon::Timeline))
		SetCommand(g_Shared, CursorCommand::RmbMenu_TaskSequence);

	const bool hasTypeSpecificOptions = g_Shared.cache.entityType == 1 || g_Shared.cache.entityType == 2;
	if (hasTypeSpecificOptions)
		CtxSeparator();
	if (g_Shared.cache.entityType == 1)
	{
		if (CtxMenuItem("Wardrobe", CursorCommand::RmbMenu_Wardrobe, ContextIcon::Shirt))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Wardrobe);
		if (CtxMenuItem("Animations", CursorCommand::RmbMenu_Animations, ContextIcon::PersonRunning))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Animations);
	}

	if (g_Shared.cache.entityType == 2)
	{
		if (CtxBeginMenu("Vehicle State", ContextIcon::Car))
		{
			if (CtxBeginMenu("Windows")) { DrawVehicleWindowsImGui(); CtxEndMenu(); }
			if (CtxBeginMenu("Doors")) { DrawVehicleDoorsImGui(); CtxEndMenu(); }
			if (CtxBeginMenu("Lights")) { DrawVehicleLightsImGui(); CtxEndMenu(); }
			if (CtxBeginMenu("Vehicle Health")) { DrawVehicleHealthImGui(); CtxEndMenu(); }
			if (CtxBeginMenu("Extras")) { DrawVehicleExtrasImGui(); CtxEndMenu(); }
			if (CtxBeginMenu("Neons")) { DrawVehicleNeonsImGui(); CtxEndMenu(); }
			if (CtxMenuItem("Toggle Engine", CursorCommand::RmbMenu_Engine, ContextIcon::None, false, false, { "Engine" }))
				SetCommand(g_Shared, CursorCommand::RmbMenu_Engine);
			CtxEndMenu();
		}
		if (CtxMenuItem("Menyoo Customs", CursorCommand::RmbMenu_MenyooCustoms, ContextIcon::Wrench))
			SetCommand(g_Shared, CursorCommand::RmbMenu_MenyooCustoms);
	}

	if (g_Shared.cache.entityAttached)
	{
		CtxSeparator();
		if (CtxMenuItem("Detach", CursorCommand::RmbMenu_Detach, ContextIcon::Unlink))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Detach);
	}
	CtxSeparator();

	if (CtxMenuItem("Copy", CursorCommand::RmbMenu_Copy, ContextIcon::Copy, false, false, {"Duplicate"}))
		SetCommand(g_Shared, CursorCommand::RmbMenu_Copy);
	if (CtxMenuItem("Delete", CursorCommand::RmbMenu_Delete, ContextIcon::Trash, false, false, {"Remove"}, true))
		SetCommand(g_Shared, CursorCommand::RmbMenu_Delete);
}

void DrawContextMenu()
{
	enum class PopupKind { None, Entity, EmptySpace };
	static PopupKind popupKind = PopupKind::None;
	static bool popupNeedsScrollbar = false;
	static EntityCache popupEntityCard{};
	static bool popupEntityCardValid = false;

	g_Shared.render.ctxSearchFocused = false;
	if (g_Shared.popupRequest == PopupRequest::Entity)
	{
		g_Shared.popupRequest = PopupRequest::None;
		g_ctxSearchBuf[0] = '\0';
		popupNeedsScrollbar = false;
		if (g_Shared.cache.entityValid)
		{
			popupKind = PopupKind::Entity;
			popupEntityCard = g_Shared.cache;
			popupEntityCardValid = true;
			ImGui::OpenPopup("spooner_ctx");
		}
	}
	else if (g_Shared.popupRequest == PopupRequest::EmptySpace)
	{
		g_Shared.popupRequest = PopupRequest::None;
		g_ctxSearchBuf[0] = '\0';
		popupNeedsScrollbar = false;
		popupKind = PopupKind::EmptySpace;
		popupEntityCardValid = false;
		ImGui::OpenPopup("spooner_ctx");
	}

	const float uiScale = ImGui::GetFontSize() / 16.0f;
	const float maxPopupHeight = ImGui::GetIO().DisplaySize.y * 0.80f;
	const ImVec4 popupBg = ImGuiTheme::ToImGuiColor(g_Shared.theme.background);
	ImGui::SetNextWindowSizeConstraints(ImVec2(280.0f * uiScale, 0.0f), ImVec2(460.0f * uiScale, maxPopupHeight));
	ImGuiMenuStyle::PopupConfig popupConfig{
		ImVec4(popupBg.x, popupBg.y, popupBg.z, 0.94f),
		WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.optionText), 0.24f),
		WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.selectionHighlight), 0.48f),
		WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.selectionHighlight), 0.72f),
		uiScale };
	ImGuiMenuStyle::ScopedPopupStyle popupStyle(popupConfig);
	const ImGuiWindowFlags popupFlags = ImGuiWindowFlags_AlwaysAutoResize |
		(popupNeedsScrollbar ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoScrollbar);
	if (!ImGui::BeginPopup("spooner_ctx", popupFlags))
	{
		if (!ImGui::IsPopupOpen("spooner_ctx"))
		{
			popupKind = PopupKind::None;
			popupEntityCardValid = false;
			g_ctxSearchBuf[0] = '\0';
		}
		return;
	}

	const ImGuiMenuStyle::Metrics menuMetrics = ImGuiMenuStyle::Metrics::FromFontSize(ImGui::GetFontSize());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(menuMetrics.itemSpacingX, menuMetrics.itemSpacingY));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(menuMetrics.framePaddingX, menuMetrics.framePaddingY));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f * uiScale);
	ImVec4 searchBg = BlendImGuiColor(ImGuiTheme::ToImGuiColor(g_Shared.theme.background), ImGuiTheme::ToImGuiColor(g_Shared.theme.titleBox), 0.35f);
	searchBg.x = (std::max)(searchBg.x, 0.12f);
	searchBg.y = (std::max)(searchBg.y, 0.12f);
	searchBg.z = (std::max)(searchBg.z, 0.13f);
	const ImVec4 subduedSeparator = BlendImGuiColor(ImGuiTheme::ToImGuiColor(g_Shared.theme.background), ImGuiTheme::ToImGuiColor(g_Shared.theme.optionBreaks), 0.30f);
	ImGui::PushStyleColor(ImGuiCol_Text, WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.optionText), 0.84f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(searchBg.x, searchBg.y, searchBg.z, 0.98f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, BlendImGuiColor(ImGuiTheme::ToImGuiColor(g_Shared.theme.background), ImGuiTheme::ToImGuiColor(g_Shared.theme.selectionHighlight), 0.38f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, BlendImGuiColor(ImGuiTheme::ToImGuiColor(g_Shared.theme.background), ImGuiTheme::ToImGuiColor(g_Shared.theme.selectionHighlight), 0.52f));
	ImGui::PushStyleColor(ImGuiCol_Border, BlendImGuiColor(ImGuiTheme::ToImGuiColor(g_Shared.theme.background), ImGuiTheme::ToImGuiColor(g_Shared.theme.optionBreaks), 0.35f));
	ImGui::PushStyleColor(ImGuiCol_Separator, subduedSeparator);
	ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, BlendImGuiColor(subduedSeparator, ImGuiTheme::ToImGuiColor(g_Shared.theme.selectionHighlight), 0.35f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

	const auto endStyledPopup = [&]()
	{
		popupNeedsScrollbar = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y > maxPopupHeight;
		ImGui::PopStyleColor(10);
		ImGui::PopStyleVar(3);
		ImGui::EndPopup();
	};

	if (!g_Shared.render.cursorModeEnabled)
	{
		ImGui::CloseCurrentPopup();
		endStyledPopup();
		return;
	}

	if (popupKind == PopupKind::EmptySpace)
	{
		DrawEmptySpaceMenu();
		endStyledPopup();
		return;
	}

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##ctxSearch", "Search...", g_ctxSearchBuf, sizeof(g_ctxSearchBuf), ImGuiInputTextFlags_EscapeClearsAll);
	g_Shared.render.ctxSearchFocused = ImGui::IsItemActive();
	if (g_IconFont)
	{
		const ImVec2 searchMin = ImGui::GetItemRectMin();
		const ImVec2 searchMax = ImGui::GetItemRectMax();
		const ImVec4 searchIconColor = WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.optionText), 0.46f);
		const ImVec2 searchIconPos(searchMax.x - g_IconFont->FontSize - 8.0f * uiScale,
			searchMin.y + (searchMax.y - searchMin.y - g_IconFont->FontSize) * 0.5f);
		ImGui::GetWindowDrawList()->AddText(g_IconFont, g_IconFont->FontSize, searchIconPos,
			ImGui::ColorConvertFloat4ToU32(searchIconColor), IconGlyph(ContextIcon::Search));
	}

	if (popupKind == PopupKind::Entity && popupEntityCardValid)
	{
		const EntityCache& card = popupEntityCard;
		const ImVec4 accent = ImGuiTheme::ToImGuiColor(g_Shared.theme.selectionHighlight);
		const char* entityIcon = card.entityType == 1 ? "\xEF\x80\x87" : card.entityType == 2 ? "\xEF\x97\xA4" : "\xEF\x91\xA6";
		const float lineHeight = ImGui::GetTextLineHeight();
		const float cardHeight = lineHeight * 2.0f + 8.0f * uiScale;
		const ImVec2 cardStart = ImGui::GetCursorScreenPos();
		const ImVec2 cardEnd(cardStart.x + ImGui::GetContentRegionAvail().x, cardStart.y + cardHeight);
		const float iconHeight = g_HeaderIconFont ? g_HeaderIconFont->FontSize : lineHeight;
		const ImVec2 iconStart(cardStart.x + 8.0f * uiScale, cardStart.y + (cardHeight - iconHeight) * 0.5f);
		ImGui::SetCursorScreenPos(iconStart);
		if (g_HeaderIconFont)
			ImGui::PushFont(g_HeaderIconFont);
		ImGui::TextColored(accent, "%s", entityIcon);
		if (g_HeaderIconFont)
			ImGui::PopFont();
		const float cardTextX = cardStart.x + 50.0f * uiScale;
		ImGui::SetCursorScreenPos(ImVec2(cardTextX, cardStart.y + 5.0f * uiScale));
		const char* entityName = card.entityHashName.empty()
			? "Selected Entity" : card.entityHashName.c_str();
		ImGui::TextColored(ImGuiTheme::ToImGuiColor(g_Shared.theme.titleText), "%s", entityName);
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, WithImGuiAlpha(ImGuiTheme::ToImGuiColor(g_Shared.theme.optionBreaks), 0.68f));
		if (g_SmallFont)
			ImGui::PushFont(g_SmallFont);
		const float subtitleHeight = g_SmallFont ? g_SmallFont->FontSize : lineHeight;
		ImGui::SetCursorScreenPos(ImVec2(cardTextX, cardEnd.y - subtitleHeight - 5.0f * uiScale));
		ImGui::TextDisabled("%s  ·  %s", EntityTypeLabel(card.entityType), card.entityInDb ? "In Database" : "Not in Database");
		if (g_SmallFont)
			ImGui::PopFont();
		ImGui::PopStyleColor();
		ImGui::SetCursorScreenPos(ImVec2(cardStart.x, cardEnd.y + 10.0f * uiScale));
	}
	else
	{
		ImGui::TextDisabled("Entity");
		CtxGroupSpacing();
	}

	g_searchPool.clear();
	g_breadcrumbStack.clear();

	if (g_ctxSearchBuf[0] != '\0')
	{
		g_searchBuildMode = true;
		DrawContextMenu_Normal();
		g_searchBuildMode = false;

		DrawContextMenu_SearchMode();
	}
	else
	{
		DrawContextMenu_Normal();
	}

	endStyledPopup();
}

}
