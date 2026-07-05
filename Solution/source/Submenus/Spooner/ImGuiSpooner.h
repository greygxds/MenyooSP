#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <atomic>
#include "..\..\Util\GTAmath.h"
#include "SpoonerMode.h"

typedef unsigned long DWORD, Hash;

struct ImGuiIO;

namespace sub::Spooner::ImGuiSpooner
{
	enum class CursorCommand : uint8_t
	{
		// Context menu commands (when clicking on an entity)
		None,
		SelectEntity,
		SelectEntityAndShowMenu,
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
		RmbMenu_Detach,
		RmbMenu_Engine,
		RmbMenu_Lights,
		RmbMenu_Repair,
		RmbMenu_MenyooCustoms,

		//  Context menu commands (when clicking on empty space)
		EmptyMenu_PlaceEntityHere,

		// Menu bar commands
		World_TimePreset,
		World_WeatherSet,
		World_WeatherReset,
		World_SpeedSet,
		SpawnFavourite,
		View_GridSnap,
		View_RotationSnap,
		View_ModeSwitch,
		View_DrawGrid,
		CloseSpooner,
	};
	
	// Favourites (FavouriteAnims.xml, FavouriteProps.xml, FavouritePeds.xml, FavouriteVehicles.xml)
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
	};

	// Gizmo writes
	struct PendingWrites
	{
		bool positionDirty = false;  Vector3 positionVal{};
		bool rotationDirty = false;  Vector3 rotationVal{};
		bool scaleDirty = false;     Vector3 scaleVal{1.0f, 1.0f, 1.0f};
	};

	// Spooner DB entry cache
	struct DbEntry { std::string hashName; int dbIndex; };
		
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
		std::string entityHashName;
		bool entityAttached = false;
		bool vehicleEngineOn = false;
		bool vehicleLightsOn = false;
	};

	// Queued command for processing in the main thread
	struct QueuedCommand
	{
		CursorCommand cmd = CursorCommand::None;
		int intPayload = 0;
		int dbPayload = -1;
		float floatPayload = 0.0f;
		FavouriteSpawnPayload spawnPayload{};
	};

	struct CommandQueue
	{
		std::vector<QueuedCommand> queue;

		// Flat payload fields (set at drain time for ProcessCursorCommand handlers)
		int commandIntPayload = 0;
		int commandDbPayload = -1;
		float commandFloatPayload = 0.0f;
		FavouriteSpawnPayload spawnPayload{};
	};

	struct SharedState
	{
		RenderState render;
		EntityCache cache;
		CommandQueue cmds;

		float cursorScreenX = 0.0f;
		float cursorScreenY = 0.0f;
		float emptyMenuCursorX = 0.0f;
		float emptyMenuCursorY = 0.0f;

		std::vector<DbEntry> dbEntityCache;
		PendingWrites pending;
		FavouriteCache favouriteCache;
	};

	extern SharedState g_Shared;
	extern std::atomic<bool> g_ContextMenuReady;
	extern std::atomic<bool> g_EmptySpaceMenuReady;

	void SetCommand(SharedState& s, CursorCommand cmd, int intP = 0, int dbP = -1, float floatP = 0.0f, FavouriteSpawnPayload spawnP = {});

	void HandleCursorModeClicks(::ImGuiIO& io);
	void DrawContextMenu();

	bool Initialize();
	void Shutdown();

	void Tick();

	void SetVisible(bool visible);
	bool IsVisible();

	int Match_Score(const char* label, const char* query);

	void DrawMenu_File(SharedState& s);
	void DrawMenu_World(SharedState& s);
	void DrawMenu_Spawn(SharedState& s);
	void DrawMenu_Entity(SharedState& s);
	void DrawMenu_View(SharedState& s);
	void DrawMenuBarWindow();
}
