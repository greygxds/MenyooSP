#include "ImGuiSpooner.h"
#include "imgui.h"
#include "SpoonerSettings.h"
#include "..\..\Scripting\World.h"
#include "..\..\Menu\Menu.h"

#include <cctype>
#include <cstring>
#include <algorithm>
#include <vector>

namespace sub::Spooner::ImGuiSpooner
{

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
			curr[static_cast<size_t>(i)] = std::min(a, std::min(b, c));
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
	int threshold = std::max(2, qlen / 3);
	if (dist <= threshold) return dist + 1;

	return -1;
}

// ── Context menu ──────────────────────────────────────────────────
void HandleCursorModeClicks(ImGuiIO& io)
{
	// Ignore clicks if the gizmo is being used
	if (g_Shared.render.gizmoUsing)
		return;

	// Handle left click (select entity), ignore left clicks in ImGui windows
	if (io.MouseClicked[0] && !io.WantCaptureMouse)
	{
		g_Shared.cursorScreenX = (io.MousePos.x / io.DisplaySize.x) * 2.0f - 1.0f;
		g_Shared.cursorScreenY = (io.MousePos.y / io.DisplaySize.y) * 2.0f - 1.0f;
		SetCommand(g_Shared, CursorCommand::SelectEntity);
	}
	// Handle right click (select entity and show context menu)
	if (io.MouseClicked[1])
	{
		g_Shared.cursorScreenX = (io.MousePos.x / io.DisplaySize.x) * 2.0f - 1.0f;
		g_Shared.cursorScreenY = (io.MousePos.y / io.DisplaySize.y) * 2.0f - 1.0f;
		g_Shared.emptyMenuCursorX = g_Shared.cursorScreenX;
		g_Shared.emptyMenuCursorY = g_Shared.cursorScreenY;
		SetCommand(g_Shared, CursorCommand::SelectEntityAndShowMenu);
	}
}

// ── Context menu search ───────────────────────────────────────────
struct SearchHit
{
	const char* label;
	std::string searchText;
	char breadcrumb[64];
	CursorCommand command;
	bool isToggle;
	bool toggleState;
};

thread_local std::vector<SearchHit> g_searchPool;
thread_local std::vector<const char*> g_breadcrumbStack;
thread_local bool g_searchBuildMode = false;

static char g_ctxSearchBuf[64] = "";

static bool CtxBeginMenu(const char* label)
{
	g_breadcrumbStack.push_back(label);
	if (g_searchBuildMode) return true;
	return ImGui::BeginMenu(label);
}

static void CtxEndMenu()
{
	if (!g_searchBuildMode) ImGui::EndMenu();
	g_breadcrumbStack.pop_back();
}

static void CtxSeparator()
{
	if (!g_searchBuildMode)
		ImGui::Separator();
}

static bool CtxMenuItem(const char* label, CursorCommand cmd, bool isToggle = false, bool toggleState = false, std::initializer_list<const char*> aliases = {})
{
	SearchHit hit;
	hit.label = label;
	hit.command = cmd;
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
	return ImGui::MenuItem(label, nullptr, isToggle ? toggleState : false);
}

static void DrawEmptySpaceMenu()
{
	ImGui::SeparatorText("Empty Space");

	if (!g_Shared.dbEntityCache.empty())
	{
		if (ImGui::BeginMenu("Place Entity Here"))
		{
			for (auto& entry : g_Shared.dbEntityCache)
			{
				if (ImGui::MenuItem(entry.hashName.c_str()))
					SetCommand(g_Shared, CursorCommand::EmptyMenu_PlaceEntityHere, 0, entry.dbIndex);
			}
			ImGui::EndMenu();
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
			if (ImGui::MenuItem(hit.label, bc, hit.isToggle ? hit.toggleState : false))
				SetCommand(g_Shared, hit.command);
		}
	}
}

static void DrawContextMenu_Normal()
{
	if (CtxMenuItem("Manual Editing", CursorCommand::RmbMenu_ManualEditing))
		SetCommand(g_Shared, CursorCommand::RmbMenu_ManualEditing);
	if (CtxMenuItem("Attachment Options", CursorCommand::RmbMenu_Attachment))
		SetCommand(g_Shared, CursorCommand::RmbMenu_Attachment);
	if (CtxMenuItem("Task Sequence", CursorCommand::RmbMenu_TaskSequence))
		SetCommand(g_Shared, CursorCommand::RmbMenu_TaskSequence);
	CtxSeparator();

	if (CtxMenuItem("Place On Ground", CursorCommand::RmbMenu_PlaceOnGround))
		SetCommand(g_Shared, CursorCommand::RmbMenu_PlaceOnGround);
	if (CtxMenuItem("Database", CursorCommand::RmbMenu_DbToggle, true, g_Shared.cache.entityInDb))
		SetCommand(g_Shared, CursorCommand::RmbMenu_DbToggle);
	if (g_Shared.cache.entityAttached)
	{
		if (CtxMenuItem("Detach", CursorCommand::RmbMenu_Detach))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Detach);
	}
	CtxSeparator();

	if (g_Shared.cache.entityType == 1)
	{
		if (CtxMenuItem("Wardrobe", CursorCommand::RmbMenu_Wardrobe))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Wardrobe);
		if (CtxMenuItem("Animations", CursorCommand::RmbMenu_Animations))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Animations);
		CtxSeparator();
	}

	if (g_Shared.cache.entityType == 2)
	{
		if (CtxMenuItem("Engine", CursorCommand::RmbMenu_Engine, true, g_Shared.cache.vehicleEngineOn))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Engine);
		if (CtxMenuItem("Lights", CursorCommand::RmbMenu_Lights, true, g_Shared.cache.vehicleLightsOn))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Lights);
		if (CtxMenuItem("Repair", CursorCommand::RmbMenu_Repair, false, false, {"Fix"}))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Repair);
		if (CtxMenuItem("Menyoo Customs", CursorCommand::RmbMenu_MenyooCustoms))
			SetCommand(g_Shared, CursorCommand::RmbMenu_MenyooCustoms);
		CtxSeparator();
	}

	if (CtxBeginMenu("Entity Flags"))
	{
		if (CtxMenuItem("Frozen In Place", CursorCommand::RmbMenu_Frozen, true, g_Shared.cache.entityFrozen, {"Freeze", "Lock"}))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Frozen);
		if (CtxMenuItem("Collision", CursorCommand::RmbMenu_Collision, true, g_Shared.cache.entityCollision, {"Physics"}))
			SetCommand(g_Shared, CursorCommand::RmbMenu_Collision);
		CtxEndMenu();
	}
	CtxSeparator();

	if (CtxMenuItem("Copy", CursorCommand::RmbMenu_Copy, false, false, {"Duplicate"}))
		SetCommand(g_Shared, CursorCommand::RmbMenu_Copy);
	if (CtxMenuItem("Delete", CursorCommand::RmbMenu_Delete, false, false, {"Remove"}))
		SetCommand(g_Shared, CursorCommand::RmbMenu_Delete);
}

void DrawContextMenu()
{
	g_Shared.render.ctxSearchFocused = false;
	bool entityPopup = g_ContextMenuReady.exchange(false) && g_Shared.cache.entityValid;
	bool emptyPopup = g_EmptySpaceMenuReady.exchange(false);
	if (entityPopup || emptyPopup)
		ImGui::OpenPopup("spooner_ctx");

	ImGui::SetNextWindowSizeConstraints(ImVec2(280, 0), ImVec2(600, 600));
	if (!ImGui::BeginPopup("spooner_ctx"))
		return;

	if (!g_Shared.render.cursorModeEnabled)
	{
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
	}

	if (emptyPopup)
	{
		DrawEmptySpaceMenu();
		ImGui::EndPopup();
		return;
	}

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##ctxSearch", "Search...", g_ctxSearchBuf, sizeof(g_ctxSearchBuf), ImGuiInputTextFlags_EscapeClearsAll);
	g_Shared.render.ctxSearchFocused = ImGui::IsItemActive();

	if (!g_Shared.cache.entityHashName.empty())
	{
		std::string header = g_Shared.cache.entityHashName;
		if (g_Shared.cache.entityInDb) header += " (DB)";
		ImGui::SeparatorText(header.c_str());
	}
	else
		ImGui::SeparatorText("Entity");

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

	ImGui::EndPopup();
}

}
