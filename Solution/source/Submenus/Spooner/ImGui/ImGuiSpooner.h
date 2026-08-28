#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "..\..\..\Util\GTAmath.h"
#include "..\SpoonerMode.h"
#include "..\..\..\UI\ImGui\MenyooTheme.h"

typedef unsigned long DWORD, Hash;

struct ImGuiIO;
struct ImFont;

namespace sub::Spooner::ImGuiSpooner
{
	extern ImFont* g_IconFont;
	extern ImFont* g_HeaderIconFont;
	extern ImFont* g_SmallFont;
	extern ImFont* g_MenuBarFont;
	enum class CursorCommand : uint8_t
	{
		// Context menu commands (when clicking on an entity)
		None,
		SelectEntity,
		SelectEntityAndShowMenu,
		SelectEntitiesInRectangle,
		RmbMenu_ManualEditing,
		RmbMenu_Attachment,
		RmbMenu_TaskSequence,
		RmbMenu_Wardrobe,
		RmbMenu_Animations,
		RmbMenu_Frozen,
		RmbMenu_Collision,
		RmbMenu_Copy,
		RmbMenu_Delete,
		RmbMenu_PlaceOnGround,
		RmbMenu_DbToggle,
		RmbMenu_FavouriteToggle,
		RmbMenu_Detach,
		RmbMenu_Engine,
		RmbMenu_Lights,
		RmbMenu_Repair,
		RmbMenu_MenyooCustoms,
		RmbMenu_SelectRadius,
		RmbMenu_WindowAction,
		RmbMenu_DoorAction,
		RmbMenu_LightToggle,
		RmbMenu_ExtraToggle,
		RmbMenu_NeonToggle,
		RmbMenu_HealthSet,

		//  Context menu commands (when clicking on empty space)
		EmptyMenu_PlaceEntityHere,

		// Menu bar commands
		World_TimePreset,
		World_WeatherSet,
		World_WeatherReset,
		World_SpeedSet,
		SpawnFavourite,
		OpenMenu,
		View_GridSnap,
		View_RotationSnap,
		View_DrawGrid,
		View_CursorMode,
		CloseSpooner,
	};
	
	// Cached spawn lists from the existing prop, ped, and vehicle XML files.
	struct FavouriteEntry
	{
		std::string name;
		Hash modelHash;
	};

	struct FavouriteCache
	{
		std::vector<FavouriteEntry> props;
		std::vector<FavouriteEntry> peds;
		std::vector<FavouriteEntry> vehicles;
	};

	struct FavouriteSpawnPayload
	{
		uint8_t category; // 0=prop, 1=ped, 2=veh
		Hash modelHash;
		std::string name;
	};

	struct SelectionRectangle
	{
		float minX = 0.0f;
		float minY = 0.0f;
		float maxX = 0.0f;
		float maxY = 0.0f;
		bool additive = false;
	};

	enum class PopupRequest : uint8_t
	{
		None,
		Entity,
		EmptySpace,
	};

	// Gizmo writes
	struct PendingWrites
	{
		int entityHandle = 0;
		bool positionDirty = false;  Vector3 positionVal{};
		bool rotationDirty = false;  Vector3 rotationVal{};
		bool scaleDirty = false;     Vector3 scaleVal{1.0f, 1.0f, 1.0f};
	};

	// Spooner DB entry cache
	struct DbEntry { std::string hashName; int entityHandle; };
		
	struct RenderState
	{
		Vector3 camCoord{};
		Vector3 camRot{};
		float   camFov = 50.0f;
		SpoonerMode::EditingState editingState;
		bool gizmoOver = false;
		bool gizmoUsing = false;
		bool cursorModeEnabled = false;
		bool ctxSearchFocused = false;
		bool gridSnapEnabled = false;
		float gridSnapSize = 1.0f;
		float rotationSnapDegrees = 0.0f;
		bool drawGrid = false;
	};

	struct EntityCache
	{
		bool entityValid = false;
		int entityHandle = 0;
		Vector3 position{};
		Vector3 rotation{};
		Vector3 scale{1.0f, 1.0f, 1.0f};
		bool entityFrozen = false;
		bool entityCollision = true;
		int  entityType = 0; // 0=unk, 1=ped, 2=veh, 3=prop
		bool entityInDb = false;
		bool entityFavourite = false;
		std::string entityHashName;
		bool entityAttached = false;
		bool vehicleDoorOpen[6] = {};
		bool multiSelectActive = false;
	};

	// Queued command for processing in the main thread
	struct QueuedCommand
	{
		CursorCommand cmd = CursorCommand::None;
		int targetEntityHandle = 0;
		int intPayload = 0;
		int dbPayload = -1;
		float floatPayload = 0.0f;
		FavouriteSpawnPayload spawnPayload{};
		float cursorScreenX = 0.0f;
		float cursorScreenY = 0.0f;
		SelectionRectangle selectionRectangle{};
	};

	struct CommandQueue
	{
		std::vector<QueuedCommand> queue;
	};

	struct SharedState
	{
		PopupRequest popupRequest = PopupRequest::None;
		ImGuiTheme::ThemeSnapshot theme;
		RenderState render;
		EntityCache cache;
		CommandQueue cmds;

		float cursorScreenX = 0.0f;
		float cursorScreenY = 0.0f;

		std::vector<DbEntry> dbEntityCache;
		PendingWrites pending;
		FavouriteCache favouriteCache;
	};

	extern SharedState g_Shared;

	void SetCommand(SharedState& state, CursorCommand command, int intPayload = 0, int dbPayload = -1, float floatPayload = 0.0f, FavouriteSpawnPayload spawnPayload = {}, SelectionRectangle selectionRectangle = {});

	void HandleCursorModeClicks(::ImGuiIO& io);
	void CancelDragSelection();
	void DrawContextMenu();

	bool Initialize();
	void Shutdown();

	void Tick();

	void SetVisible(bool visible);
	void SetCursorModeEnabled(bool enabled);
	bool IsVisible();

	int Match_Score(const char* label, const char* query);

	void DrawMenu_File(SharedState& s);
	void DrawMenu_World(SharedState& s);
	void DrawMenu_Spawn(SharedState& s);
	void DrawMenu_Entity(SharedState& s);
	void DrawMenu_View(SharedState& s);
	void DrawMenuBarWindow();
}
