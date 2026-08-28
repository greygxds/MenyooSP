#include "ImGuiSpooner_Internal.h"

#include "..\SpoonerEntity.h"
#include "..\SpoonerMode.h"
#include "..\SpoonerSettings.h"
#include "..\Submenus.h"
#include "..\EntityManagement.h"
#include "..\Databases.h"
#include "..\FavouritesManagement.h"
#include "..\..\PedModelChanger.h"

#include "..\..\..\Menu\submenu_enum.h"
#include "..\..\..\Scripting\Camera.h"
#include "..\..\..\Scripting\Game.h"
#include "..\..\..\Scripting\Model.h"
#include "..\..\..\Scripting\World.h"
#include "..\..\..\Scripting\GTAvehicle.h"
#include "..\..\..\Natives\natives2.h"
#include "..\..\..\Menu\Routine.h"

#include <algorithm>
#include <iterator>

namespace sub::Spooner::ImGuiSpooner
{
	namespace
	{
		struct PendingSpawn
		{
			Hash modelHash = 0;
			uint8_t category = 0;
			std::string name;
			DWORD startTime = 0;
			bool active = false;
		};

		PendingSpawn g_PendingSpawn;
		bool g_leftIndicator = false, g_rightIndicator = false, g_hazardLights = false;

		using CmdHandler = void(*)(const QueuedCommand&);

		void Cmd_None(const QueuedCommand&) {}

		void Cmd_RmbMenu_ManualEditing(const QueuedCommand&) { SpoonerMode::OpenMenu(SUB::SPOONER_MANUALEDITING); }
		void Cmd_RmbMenu_Attachment(const QueuedCommand&) { SpoonerMode::OpenMenu(SUB::SPOONER_ATTACHMENTOPS); }
		void Cmd_RmbMenu_TaskSequence(const QueuedCommand&) { SpoonerMode::OpenMenu(SUB::SPOONER_TASKSEQUENCE_TASKLIST); }
		void Cmd_RmbMenu_Wardrobe(const QueuedCommand&) { Submenus::SetSelectedEntityAsActivePed(); SpoonerMode::OpenMenu(SUB::COMPONENTS); }
		void Cmd_RmbMenu_Animations(const QueuedCommand&) { Submenus::SetSelectedEntityAsActivePed(); SpoonerMode::OpenMenu(SUB::ANIMATIONSUB); }
		void Cmd_RmbMenu_Frozen(const QueuedCommand&)
		{
			if (selectedEntity.handle.Exists())
				selectedEntity.handle.FreezePosition(!selectedEntity.handle.IsPositionFrozen());
		}
		void Cmd_RmbMenu_Collision(const QueuedCommand&)
		{
			if (selectedEntity.handle.Exists())
				selectedEntity.handle.SetIsCollisionEnabled(!selectedEntity.handle.GetIsCollisionEnabled());
		}
		void Cmd_RmbMenu_Copy(const QueuedCommand&)
		{
			if (!selectedEntity.handle.Exists()) return;
			selectedEntity = EntityManagement::CopyEntity(
				selectedEntity,
				EntityManagement::GetEntityIndexInDb(selectedEntity) >= 0,
				true,
				Submenus::_copyEntTexterValue);
		}
		void Cmd_RmbMenu_Delete(const QueuedCommand&)
		{
			if (!selectedEntity.handle.Exists()) return;
			selectedEntity.handle.RequestControl(600);
			EntityManagement::DeleteEntity(selectedEntity);
			SpoonerMode::ResetSelectedEntity();
			SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
		}
		void Cmd_RmbMenu_PlaceOnGround(const QueuedCommand&)
		{
			if (selectedEntity.handle.Exists()) selectedEntity.handle.PlaceOnGround();
		}
		void Cmd_RmbMenu_DbToggle(const QueuedCommand&)
		{
			if (!selectedEntity.handle.Exists()) return;
			const int index = EntityManagement::GetEntityIndexInDb(selectedEntity);
			if (index >= 0)
				EntityManagement::RemoveEntityFromDb(selectedEntity);
			else
				EntityManagement::AddEntityToDb(selectedEntity, Settings::bAddToDbAsMissionEntities);
		}
		void Cmd_RmbMenu_FavouriteToggle(const QueuedCommand&)
		{
			if (!selectedEntity.handle.Exists()) return;
			const GTAmodel::Model model = selectedEntity.handle.Model();
			switch (static_cast<EntityType>(selectedEntity.handle.Type()))
			{
			case EntityType::PROP:
				if (FavouritesManagement::IsPropAFavourite(selectedEntity.hashName, model.hash))
					FavouritesManagement::RemovePropFromFavourites(selectedEntity.hashName, model.hash);
				else
					FavouritesManagement::AddPropToFavourites(selectedEntity.hashName, model.hash);
				break;
			case EntityType::PED:
				if (PedFavourites::IsPedAFavourite(model))
					PedFavourites::RemovePedFromFavourites(model);
				else
				{
					const std::string name = Game::InputBox("", 28U, "Enter custom name:", selectedEntity.hashName);
					if (!name.empty()) PedFavourites::AddPedToFavourites(model, name);
				}
				break;
			case EntityType::VEHICLE:
				if (FavouritesManagement::IsVehicleAFavourite(model))
					FavouritesManagement::RemoveVehicleFromFavourites(model);
				else
				{
					const std::string name = Game::InputBox("", 28U, "Enter custom name:", selectedEntity.hashName);
					if (!name.empty()) FavouritesManagement::AddVehicleToFavourites(model, name);
				}
				break;
			default: break;
			}
		}
		void Cmd_RmbMenu_Detach(const QueuedCommand&)
		{
			if (selectedEntity.handle.Exists()) EntityManagement::DetachEntity(selectedEntity);
		}
		void Cmd_RmbMenu_Engine(const QueuedCommand&)
		{
			if (!selectedEntity.handle.Exists() || static_cast<EntityType>(selectedEntity.handle.Type()) != EntityType::VEHICLE) return;
			const BOOL running = GET_IS_VEHICLE_ENGINE_RUNNING(selectedEntity.handle.Handle());
			SET_VEHICLE_ENGINE_ON(selectedEntity.handle.Handle(), !running, true, true);
		}
		void Cmd_RmbMenu_Lights(const QueuedCommand&)
		{
			if (!selectedEntity.handle.Exists() || static_cast<EntityType>(selectedEntity.handle.Type()) != EntityType::VEHICLE) return;
			BOOL lightsOn = FALSE, highbeamsOn = FALSE;
			GET_VEHICLE_LIGHTS_STATE(selectedEntity.handle.Handle(), &lightsOn, &highbeamsOn);
			SET_VEHICLE_LIGHTS(selectedEntity.handle.Handle(), lightsOn ? 4 : 3);
		}
		void Cmd_RmbMenu_Repair(const QueuedCommand&)
		{
			if (selectedEntity.handle.Exists() && static_cast<EntityType>(selectedEntity.handle.Type()) == EntityType::VEHICLE)
				SET_VEHICLE_FIXED(selectedEntity.handle.Handle());
		}
		void Cmd_RmbMenu_MenyooCustoms(const QueuedCommand&)
		{
			Submenus::SetSelectedEntityAsVehicleTarget();
			SpoonerMode::OpenMenu(SUB::MODSHOP);
		}
		void Cmd_RmbMenu_SelectRadius(const QueuedCommand& command)
		{
			if (!selectedEntity.handle.Exists()) return;
			const float radius = command.floatPayload > 0.0f ? command.floatPayload : 10.0f;
			const Vector3 center = selectedEntity.handle.GetPosition();
			Submenus::MultiSelect::DestroyPivot();
			Submenus::MultiSelect::Clear();
			for (const auto& entry : Databases::EntityDb)
			{
				if (!entry.handle.Exists() || center.DistanceTo(entry.handle.GetPosition()) > radius)
					continue;
				Submenus::MultiSelect::Add(entry);
			}
			SpoonerMode::OpenMenu(SUB::SPOONER_MULTISELECT);
		}
		void Cmd_RmbMenu_WindowAction(const QueuedCommand& command)
		{
			if (!selectedEntity.handle.Exists() || !selectedEntity.handle.IsVehicle()) return;
			GTAvehicle vehicle = selectedEntity.handle;
			const int action = command.intPayload / 10, index = command.intPayload % 10;
			const int first = index == 4 ? 0 : index, last = index == 4 ? 4 : index + 1;
			for (int i = first; i < last; ++i)
			{
				const auto window = static_cast<VehicleWindow>(i);
				vehicle.RequestControl();
				switch (action) { case 0: vehicle.RollDownWindow(window); break; case 1: vehicle.RollUpWindow(window); break;
				case 2: vehicle.SmashWindow(window); break; case 3: vehicle.FixWindow(window); break; case 4: vehicle.RemoveWindow(window); break; }
			}
		}
		void Cmd_RmbMenu_DoorAction(const QueuedCommand& command)
		{
			if (!selectedEntity.handle.Exists() || !selectedEntity.handle.IsVehicle()) return;
			GTAvehicle vehicle = selectedEntity.handle;
			const int action = command.intPayload / 10, index = command.intPayload % 10;
			const int first = index == 7 ? 0 : index, last = index == 7 ? 7 : index + 1;
			for (int i = first; i < last; ++i)
			{
				const auto door = static_cast<VehicleDoor>(i); vehicle.RequestControl();
				switch (action) { case 0: vehicle.IsDoorOpen(door) ? vehicle.CloseDoor(door, false) : vehicle.OpenDoor(door, false, false); break;
				case 1: vehicle.CloseDoor(door, false); break; case 2: vehicle.OpenDoor(door, false, false); break; case 3: vehicle.BreakDoor(door, true); break; case 4: vehicle.FixDoor(door); break; }
			}
		}
		void Cmd_RmbMenu_LightToggle(const QueuedCommand& command)
		{
			if (!selectedEntity.handle.Exists() || !selectedEntity.handle.IsVehicle()) return;
			GTAvehicle vehicle = selectedEntity.handle; vehicle.RequestControl();
			if (command.intPayload == 0) vehicle.SetLightsOn(!vehicle.GetLightsOn());
			else if (command.intPayload == 1) { g_leftIndicator = !g_leftIndicator; g_rightIndicator = g_hazardLights = false; vehicle.SetLeftIndicatorLightOn(g_leftIndicator); vehicle.SetRightIndicatorLightOn(false); }
			else if (command.intPayload == 2) { g_rightIndicator = !g_rightIndicator; g_leftIndicator = g_hazardLights = false; vehicle.SetRightIndicatorLightOn(g_rightIndicator); vehicle.SetLeftIndicatorLightOn(false); }
			else { g_hazardLights = !g_hazardLights; g_leftIndicator = g_rightIndicator = false; vehicle.SetLeftIndicatorLightOn(g_hazardLights); vehicle.SetRightIndicatorLightOn(g_hazardLights); }
		}
		void Cmd_RmbMenu_ExtraToggle(const QueuedCommand& command)
		{
			if (!selectedEntity.handle.Exists() || !selectedEntity.handle.IsVehicle()) return;
			GTAvehicle vehicle = selectedEntity.handle; if (vehicle.DoesExtraExist(command.intPayload)) vehicle.SetExtraOn(command.intPayload, !vehicle.GetExtraOn(command.intPayload));
		}
		void Cmd_RmbMenu_NeonToggle(const QueuedCommand& command)
		{
			if (!selectedEntity.handle.Exists() || !selectedEntity.handle.IsVehicle()) return;
			GTAvehicle vehicle = selectedEntity.handle; const auto light = static_cast<VehicleNeonLight>(command.intPayload);
			vehicle.SetNeonLightOn(light, !vehicle.IsNeonLightOn(light));
		}
		void Cmd_RmbMenu_HealthSet(const QueuedCommand& command)
		{
			if (!selectedEntity.handle.Exists() || !selectedEntity.handle.IsVehicle()) return;
			GTAvehicle vehicle = selectedEntity.handle; vehicle.RequestControl();
			switch (command.intPayload) { case 0: vehicle.SetBodyHealth(command.floatPayload); break; case 1: vehicle.SetEngineHealth(command.floatPayload); break; case 2: vehicle.SetPetrolTankHealth(command.floatPayload); break; }
		}

		void Cmd_World_TimePreset(const QueuedCommand& command)
		{
			static const int timePresets[4][2] = {{6, 0}, {12, 0}, {19, 0}, {23, 0}};
			const int index = command.intPayload;
			if (index < 0 || index >= 4) return;
			NETWORK_OVERRIDE_CLOCK_TIME(timePresets[index][0], timePresets[index][1], 0);
			if (pauseClock)
			{
				pauseClockH = static_cast<UINT8>(timePresets[index][0]);
				pauseClockM = static_cast<UINT8>(timePresets[index][1]);
			}
		}
		void Cmd_World_WeatherSet(const QueuedCommand& command)
		{
			const int index = command.intPayload;
			if (index >= 0 && index < static_cast<int>(World::sWeatherNames.size()))
				World::SetWeather(World::sWeatherNames[index].second);
		}
		void Cmd_World_WeatherReset(const QueuedCommand&) { World::ClearWeatherOverride(); }
		void Cmd_World_SpeedSet(const QueuedCommand& command) { SET_TIME_SCALE(command.floatPayload); }

		void Cmd_SpawnFavourite(const QueuedCommand& command)
		{
			if (g_PendingSpawn.active) return;
			REQUEST_MODEL(command.spawnPayload.modelHash);
			g_PendingSpawn = { command.spawnPayload.modelHash, command.spawnPayload.category,
				command.spawnPayload.name, GetTickCount(), true };
		}

		void Cmd_OpenMenu(const QueuedCommand& command) { SpoonerMode::OpenMenu(command.intPayload, command.dbPayload); }
		void Cmd_View_GridSnap(const QueuedCommand& command)
		{
			Settings::bGridSnapEnabled = command.floatPayload > 0.0f;
			if (Settings::bGridSnapEnabled) Settings::gridSnapSize = command.floatPayload;
		}
		void Cmd_View_RotationSnap(const QueuedCommand& command) { Settings::rotationSnapDegrees = command.floatPayload; }
		void Cmd_View_DrawGrid(const QueuedCommand&) { Settings::bDrawGrid = !Settings::bDrawGrid; }
		void Cmd_View_CursorMode(const QueuedCommand& command)
		{
			Settings::bCursorMode = command.intPayload != 0;
			SetCursorModeEnabled(Settings::bCursorMode);
			if (!Settings::bCursorMode) SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
		}
		void Cmd_CloseSpooner(const QueuedCommand&) { SpoonerMode::TurnOff(); }

		void Cmd_SelectEntity(const QueuedCommand& command)
		{
			if (!SpoonerCamera::camera.Exists()) return;
			GTAentity clicked = SpoonerCamera::camera.RaycastForEntity(Vector2(command.cursorScreenX, command.cursorScreenY), 0, 160.0f);
			if (!clicked.Exists())
			{
				SpoonerMode::ResetSelectedEntity();
				SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
				return;
			}
			SpoonerMode::SetAsSelectedEntity(clicked);
			SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Gizmo;
			SpoonerMode::editingState.transformMode = SpoonerMode::eTransformMode::Position;
		}
		void Cmd_SelectEntitiesInRectangle(const QueuedCommand& command)
		{
			const SelectionRectangle& rect = command.selectionRectangle;
			if (rect.minX < 0.0f || rect.minY < 0.0f || rect.maxX > 1.0f || rect.maxY > 1.0f ||
				rect.minX > rect.maxX || rect.minY > rect.maxY)
				return;

			Submenus::MultiSelect::DestroyPivot();
			if (!rect.additive)
				Submenus::MultiSelect::Clear();

			for (const auto& entry : Databases::EntityDb)
			{
				if (!entry.handle.Exists())
					continue;

				Vector2 screenPosition;
				if (!World::WorldToScreen(entry.handle.GetPosition(), screenPosition))
					continue;

				if (screenPosition.x >= rect.minX && screenPosition.x <= rect.maxX &&
					screenPosition.y >= rect.minY && screenPosition.y <= rect.maxY)
					Submenus::MultiSelect::Add(entry);
			}

			SpoonerMode::OpenMenu(SUB::SPOONER_MULTISELECT);
		}
		void Cmd_SelectEntityAndShowMenu(const QueuedCommand& command)
		{
			if (!SpoonerCamera::camera.Exists()) return;
			GTAentity clicked = SpoonerCamera::camera.RaycastForEntity(Vector2(command.cursorScreenX, command.cursorScreenY), 0, 160.0f);
			if (clicked.Exists())
			{
				SpoonerMode::SetAsSelectedEntity(clicked);
				SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
				return;
			}
			SpoonerMode::ResetSelectedEntity();
			SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
		}
		void Cmd_EmptyMenu_PlaceEntityHere(const QueuedCommand& command)
		{
			if (!SpoonerCamera::camera.Exists()) return;
			const int targetHandle = command.dbPayload;
			auto entity = std::find_if(Databases::EntityDb.begin(), Databases::EntityDb.end(),
				[targetHandle](const SpoonerEntity& entry) { return entry.handle.GetHandle() == targetHandle; });
			if (entity == Databases::EntityDb.end() || !entity->handle.Exists()) return;
			if (entity->attachmentArgs.isAttached) EntityManagement::DetachEntity(*entity);
			const Vector3 cursorPosition = SpoonerCamera::camera.RaycastForCoord(Vector2(command.cursorScreenX, command.cursorScreenY), 0, 300.0f, 300.0f);
			entity->handle.RequestControlOnce();
			entity->handle.SetPosition(cursorPosition);
			entity->handle.PlaceOnGround();
		}

		const CmdHandler s_cmdHandlers[] = {
			Cmd_None, Cmd_SelectEntity, Cmd_SelectEntityAndShowMenu, Cmd_SelectEntitiesInRectangle,
			Cmd_RmbMenu_ManualEditing, Cmd_RmbMenu_Attachment, Cmd_RmbMenu_TaskSequence,
			Cmd_RmbMenu_Wardrobe, Cmd_RmbMenu_Animations, Cmd_RmbMenu_Frozen,
			Cmd_RmbMenu_Collision, Cmd_RmbMenu_Copy, Cmd_RmbMenu_Delete,
			Cmd_RmbMenu_PlaceOnGround, Cmd_RmbMenu_DbToggle, Cmd_RmbMenu_FavouriteToggle,
			Cmd_RmbMenu_Detach,
			Cmd_RmbMenu_Engine, Cmd_RmbMenu_Lights, Cmd_RmbMenu_Repair,
			Cmd_RmbMenu_MenyooCustoms, Cmd_RmbMenu_SelectRadius,
			Cmd_RmbMenu_WindowAction, Cmd_RmbMenu_DoorAction, Cmd_RmbMenu_LightToggle,
			Cmd_RmbMenu_ExtraToggle, Cmd_RmbMenu_NeonToggle, Cmd_RmbMenu_HealthSet,
			Cmd_EmptyMenu_PlaceEntityHere,
			Cmd_World_TimePreset, Cmd_World_WeatherSet, Cmd_World_WeatherReset,
			Cmd_World_SpeedSet, Cmd_SpawnFavourite, Cmd_OpenMenu, Cmd_View_GridSnap,
			Cmd_View_RotationSnap, Cmd_View_DrawGrid, Cmd_View_CursorMode,
			Cmd_CloseSpooner,
		};
		static_assert(std::size(s_cmdHandlers) == static_cast<size_t>(CursorCommand::CloseSpooner) + 1,
			"Cursor command table is out of sync");
	}

	void CheckPendingSpawns_ScriptThread()
	{
		if (!g_PendingSpawn.active) return;
		if (GetTickCount() - g_PendingSpawn.startTime > 3000)
		{
			GTAmodel::Model(g_PendingSpawn.modelHash).Unload();
			g_PendingSpawn.active = false;
			Game::Print::PrintBottomLeft("~r~Spawn failed:~s~ model timed out");
			return;
		}
		REQUEST_MODEL(g_PendingSpawn.modelHash);
		if (!HAS_MODEL_LOADED(g_PendingSpawn.modelHash)) return;
		const GTAmodel::Model model(g_PendingSpawn.modelHash);
		switch (g_PendingSpawn.category)
		{
		case 0: EntityManagement::AddProp(model, g_PendingSpawn.name); break;
		case 1: EntityManagement::AddPed(model, g_PendingSpawn.name); break;
		case 2: EntityManagement::AddVehicle(model, g_PendingSpawn.name); break;
		}
		g_PendingSpawn.active = false;
	}

	PopupRequest ProcessCursorCommands(const std::vector<QueuedCommand>& commands)
	{
		PopupRequest popupRequest = PopupRequest::None;
		for (const auto& command : commands)
		{
			if (command.cmd == CursorCommand::None) continue;
			const int index = static_cast<int>(command.cmd);
			if (index >= 0 && index < static_cast<int>(std::size(s_cmdHandlers)))
			{
				if (index >= static_cast<int>(CursorCommand::RmbMenu_ManualEditing) &&
					index <= static_cast<int>(CursorCommand::RmbMenu_SelectRadius) &&
					command.targetEntityHandle != 0 &&
					selectedEntity.handle.GetHandle() != command.targetEntityHandle)
					continue;
				s_cmdHandlers[index](command);
			}

			if (command.cmd == CursorCommand::SelectEntityAndShowMenu && SpoonerCamera::camera.Exists())
				popupRequest = selectedEntity.handle.Exists() ? PopupRequest::Entity : PopupRequest::EmptySpace;
		}
		return popupRequest;
	}
}
