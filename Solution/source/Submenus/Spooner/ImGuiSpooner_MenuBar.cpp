// TODO:
// - fix camera lock not working correctly with gizmo mode

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

	static void DrawFavSubmenu(SharedState& s, const std::vector<FavouriteEntry>& entries, const char* label, uint8_t category)
	{
		ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (ImGui::BeginMenu(label))
		{
			if (entries.empty())
			{
				ImGui::TextDisabled("No favourites saved");
			}
			else
			{
				ImGui::BeginChild("favs", ImVec2(0.0f, 400.0f));
				for (auto& e : entries)
				{
					if (ImGui::MenuItem(e.name.c_str()))
						SetCommand(s, CursorCommand::SpawnFavourite, 0, -1, 0.0f, FavouriteSpawnPayload{ category, e.modelHash });
				}
				ImGui::EndChild();
			}
			ImGui::EndMenu();
		}
	}

	void DrawMenu_File(SharedState& s)
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save Database..."))
			{
				SetCommand(s, CursorCommand::None);
				Menu::NewSetMenu(SUB::SPOONER_SAVEFILES);
			}
			if (ImGui::MenuItem("Load..."))
			{
				SetCommand(s, CursorCommand::None);
				Menu::NewSetMenu(SUB::SPOONER_SAVEFILES);
			}
			if (ImGui::MenuItem("Close Spooner"))
				SetCommand(s, CursorCommand::CloseSpooner);
			ImGui::EndMenu();
		}
	}

	void DrawMenu_World(SharedState& s)
	{
		if (ImGui::BeginMenu("World"))
		{
			if (ImGui::BeginMenu("Time Presets"))
			{
				if (ImGui::MenuItem("Sunrise"))
					SetCommand(s, CursorCommand::World_TimePreset, 0);
				if (ImGui::MenuItem("Noon"))
					SetCommand(s, CursorCommand::World_TimePreset, 1);
				if (ImGui::MenuItem("Sunset"))
					SetCommand(s, CursorCommand::World_TimePreset, 2);
				if (ImGui::MenuItem("Night"))
					SetCommand(s, CursorCommand::World_TimePreset, 3);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Weather"))
			{
				for (int i = 0; i < (int)World::sWeatherNames.size(); i++)
				{
					if (ImGui::MenuItem(World::sWeatherNames[i].first.c_str()))
						SetCommand(s, CursorCommand::World_WeatherSet, i);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Reset"))
					SetCommand(s, CursorCommand::World_WeatherReset);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("World Speed"))
			{
				if (ImGui::MenuItem("0.1x"))
					SetCommand(s, CursorCommand::World_SpeedSet, 0, -1, 0.1f);
				if (ImGui::MenuItem("0.5x"))
					SetCommand(s, CursorCommand::World_SpeedSet, 0, -1, 0.5f);
				if (ImGui::MenuItem("1.0x"))
					SetCommand(s, CursorCommand::World_SpeedSet, 0, -1, 1.0f);
				if (ImGui::MenuItem("2.0x"))
					SetCommand(s, CursorCommand::World_SpeedSet, 0, -1, 2.0f);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
	}

	void DrawMenu_Spawn(SharedState& s)
	{
		if (ImGui::BeginMenu("Spawn"))
		{
			DrawFavSubmenu(s, s.favouriteCache.props, "Props", 0);
			DrawFavSubmenu(s, s.favouriteCache.peds, "Peds", 1);
			DrawFavSubmenu(s, s.favouriteCache.vehicles, "Vehicles", 2);
			ImGui::Separator();
			if (ImGui::MenuItem("More..."))
			{
				SetCommand(s, CursorCommand::None);
				Menu::NewSetMenu(SUB::SPOONER_SPAWN_CATEGORIES);
			}
			ImGui::EndMenu();
		}
	}

	void DrawMenu_Entity(SharedState& s)
	{
		if (ImGui::BeginMenu("Entity"))
		{
			if (ImGui::MenuItem("Manual Editing"))
				SetCommand(s, CursorCommand::RmbMenu_ManualEditing);
			if (ImGui::MenuItem("Attachment Options"))
				SetCommand(s, CursorCommand::RmbMenu_Attachment);
			if (ImGui::MenuItem("Task Sequence"))
				SetCommand(s, CursorCommand::RmbMenu_TaskSequence);
			ImGui::Separator();
			if (s.cache.entityType == 1)
			{
				if (ImGui::MenuItem("Wardrobe"))
					SetCommand(s, CursorCommand::RmbMenu_Wardrobe);
				if (ImGui::MenuItem("Animations"))
					SetCommand(s, CursorCommand::RmbMenu_Animations);
				ImGui::Separator();
			}
			if (ImGui::MenuItem("Frozen In Place", nullptr, s.cache.entityFrozen))
				SetCommand(s, CursorCommand::RmbMenu_Frozen);
			if (ImGui::MenuItem("Collision", nullptr, s.cache.entityCollision))
				SetCommand(s, CursorCommand::RmbMenu_Collision);
			ImGui::Separator();
			if (ImGui::MenuItem("Copy"))
				SetCommand(s, CursorCommand::RmbMenu_Copy);
			if (ImGui::MenuItem("Delete"))
				SetCommand(s, CursorCommand::RmbMenu_Delete);
			ImGui::EndMenu();
		}
	}

	void DrawMenu_View(SharedState& s)
	{
		if (ImGui::BeginMenu("View"))
		{
			bool gridSnap = Settings::bGridSnapEnabled;
			if (ImGui::MenuItem("Grid Snap", nullptr, &gridSnap))
			{
				SetCommand(s, CursorCommand::View_GridSnap, 0, -1, gridSnap ? Settings::gridSnapSize : 0.0f);
			}
			if (ImGui::BeginMenu("Snap Size"))
			{
				float sizes[] = {0.5f, 1.0f, 2.0f, 5.0f};
				const char* labels[] = {"0.5m", "1.0m", "2.0m", "5.0m"};
				for (int i = 0; i < 4; i++)
				{
					if (ImGui::MenuItem(labels[i], nullptr, Settings::gridSnapSize == sizes[i]))
					{
						SetCommand(s, CursorCommand::View_GridSnap, 0, -1, sizes[i]);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Rotation Snap"))
			{
				float angles[] = {0.0f, 15.0f, 30.0f, 45.0f, 90.0f};
				const char* labels[] = {"Off", "15deg", "30deg", "45deg", "90deg"};
				for (int i = 0; i < 5; i++)
				{
					if (ImGui::MenuItem(labels[i], nullptr, Settings::rotationSnapDegrees == angles[i]))
					{
						SetCommand(s, CursorCommand::View_RotationSnap, 0, -1, angles[i]);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Display Grid", nullptr, Settings::bDrawGrid))
				SetCommand(s, CursorCommand::View_DrawGrid);
			if (ImGui::BeginMenu("Mode"))
			{
				if (ImGui::MenuItem("Ground Ease", nullptr, Settings::spoonerModeMode == eSpoonerModeMode::GroundEase))
					SetCommand(s, CursorCommand::View_ModeSwitch, 0);
				if (ImGui::MenuItem("Precision", nullptr, Settings::spoonerModeMode == eSpoonerModeMode::Precision))
					SetCommand(s, CursorCommand::View_ModeSwitch, 1);
				ImGui::EndMenu();
			}
			bool cursorMode = Settings::bCursorMode;
			if (ImGui::MenuItem("Cursor Mode", nullptr, &cursorMode))
				Settings::bCursorMode = cursorMode;
			ImGui::EndMenu();
		}
	}

	static void StatusDot(const char* label, bool on)
	{
		// green dot for on, red dot for off
		ImGui::PushStyleColor(ImGuiCol_Text, on ? IM_COL32(100, 220, 100, 255) : IM_COL32(220, 100, 100, 255));
		ImGui::TextDisabled("·");
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 2);
		ImGui::TextDisabled(label);
	}

	static void DrawStatusIndicators()
	{
		float windowWidth = ImGui::GetWindowWidth();
		ImVec2 tsBase = ImGui::CalcTextSize("· Cursor  |  · GE");
		float entWidth = g_Shared.cache.entityValid && !g_Shared.cache.entityHashName.empty()
			? ImGui::CalcTextSize((g_Shared.cache.entityHashName + (g_Shared.cache.entityInDb ? " (DB)" : "")).c_str()).x + 16.0f
			: 0.0f;
		ImGui::SetCursorPosX(windowWidth - tsBase.x - entWidth - ImGui::GetStyle().WindowPadding.x);
		StatusDot("Cursor", Settings::bCursorMode);
		ImGui::SameLine(0, 4); ImGui::TextDisabled("|"); ImGui::SameLine(0, 4);
		StatusDot("GE", Settings::spoonerModeMode == eSpoonerModeMode::GroundEase);
		if (g_Shared.cache.entityValid && !g_Shared.cache.entityHashName.empty())
		{
			ImGui::SameLine(0, 4); ImGui::TextDisabled("|"); ImGui::SameLine(0, 4);
			std::string entLabel = g_Shared.cache.entityHashName;
			if (g_Shared.cache.entityInDb)
				entLabel += " (DB)";
			ImGui::TextDisabled("%s", entLabel.c_str());
		}
	}

	void DrawMenuBarWindow()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));

		ImGuiIO& io = ImGui::GetIO();
		float menuBarHeight = ImGui::GetFrameHeight();
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, menuBarHeight));
		ImGui::Begin("##spoonerMenuBar", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_MenuBar);

		ImGui::PopStyleVar(3);

		if (ImGui::BeginMenuBar())
		{
			ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Spooner"); ImGui::SameLine();
			ImGui::TextDisabled("|"); ImGui::SameLine();

			DrawMenu_File(g_Shared);
			DrawMenu_World(g_Shared);
			DrawMenu_Spawn(g_Shared);
			if (g_Shared.cache.entityValid)
				DrawMenu_Entity(g_Shared);
			DrawMenu_View(g_Shared);

			DrawStatusIndicators();

			ImGui::EndMenuBar();
		}
		ImGui::End();
		ImGui::PopStyleVar(2);
	}

}
