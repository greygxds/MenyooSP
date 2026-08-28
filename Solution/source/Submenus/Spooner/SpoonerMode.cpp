/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "SpoonerMode.h"

#include "ImGui/ImGuiSpooner.h"
#include "..\..\macros.h"

#include "..\..\Menu\Menu.h"
//#include "..\..\Menu\Routine.h"

#include "..\..\Natives\natives2.h"
#include "..\..\Util\keyboard.h"
#include "..\..\Scripting\Camera.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\GTAprop.h"
#include "..\..\Scripting\GTAvehicle.h"
#include "..\..\Scripting\GTAped.h"
#include "..\..\Scripting\GTAplayer.h"
#include "..\..\Util\GTAmath.h"
#include "..\..\Natives\types.h" //RGBA
#include "..\..\Scripting\World.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Scripting\ModelNames.h"
#include "..\..\Util\StringManip.h"
#include "..\..\Scripting\enums.h"
#include "..\..\Scripting\Game.h"

#include "SpoonerSettings.h"
#include "EntityManagement.h"
#include "SpoonerEntity.h"
#include "Databases.h"
#include "SpoonerMarker.h"
#include "MarkerManagement.h"
#include "SpoonerLight.h"
#include "Submenus.h"
#include "..\\..\\Memory\\GTAmemory.h"

#include <utility>
#include <set>
#include <algorithm>
#include <math.h>
#include <Menu/Routine.h>

namespace sub::Spooner
{
	namespace SpoonerMode
	{
		namespace
		{
			struct NativeCursorState
			{
				int hoveredEntityHandle = 0;
				std::string hoveredEntityName;
				bool dragging = false;
				int draggedEntityHandle = 0;
				bool draggedEntityHadCollision = true;
				float visibilityOpacity = 0.0f;
				float hoverOpacity = 0.0f;
				float textOpacity = 0.0f;
				DWORD lastUpdateTime = 0;
			};

			NativeCursorState nativeCursor;

			constexpr float nativeCursorDotSize = 0.008f;
			constexpr float nativeCursorTextY = 0.515f;
			constexpr float nativeCursorInstructionY = 0.54f;

			bool ShouldDrawNativeCursor()
			{
				return bEnabled && !Settings::bCursorMode &&
					Menu::activeSubmenu == SUB::CLOSED &&
					editingState.mode == eEditMode::Disabled &&
					SpoonerCamera::camera.Exists();
			}

			GTAentity GetNativeCursorEntity()
			{
				return ShouldDrawNativeCursor()
					? SpoonerCamera::camera.RaycastForEntity(Vector2(0.0f, 0.0f), 0, 160.0f)
					: GTAentity();
			}

			std::string GetNativeCursorEntityName(GTAentity entity)
			{
				SpoonerEntity* entityInfo = nullptr;
				const bool isInDatabase = GetEntityPtr(entity, entityInfo);
				const std::string name = entityInfo != nullptr ? entityInfo->hashName : std::string();
				if (!isInDatabase)
					delete entityInfo;
				return name;
			}

			float AnimateNativeCursorOpacity(float current, float target, float deltaSeconds, float duration)
			{
				const float step = duration > 0.0f ? deltaSeconds / duration : 1.0f;
				return current < target
					? (std::min)(target, current + step)
					: (std::max)(target, current - step);
			}

			void BeginNativeEntityDrag(GTAentity entity)
			{
				if (nativeCursor.dragging || !entity.Exists()) return;

				nativeCursor.dragging = true;
				nativeCursor.draggedEntityHandle = entity.Handle();
				SetAsSelectedEntity(entity);
				nativeCursor.draggedEntityHadCollision = selectedEntity.handle.GetIsCollisionEnabled();
				selectedEntity.handle.RequestControl();
				selectedEntity.handle.SetIsCollisionEnabled(false);
			}

			void UpdateNativeEntityDrag()
			{
				if (!nativeCursor.dragging) return;

				GTAentity entity(nativeCursor.draggedEntityHandle);
				if (!entity.Exists()) return;

				entity.RequestControl();
				const Vector3 rotation = entity.Rotation_get();
				const ModelDimensions& dimensions = entity.ModelDimensions();
				float groundOffset = dimensions.Dim1.z;
				if (fabs(rotation.x) > 150.0f || fabs(rotation.y) > 150.0f)
					groundOffset = dimensions.Dim2.z;
				else if (fabs(rotation.x) > 70.0f && fabs(rotation.y) > 70.0f)
					groundOffset = (dimensions.Dim1.y + dimensions.Dim1.x) / 2.0f;
				else if (fabs(rotation.x) > 70.0f)
					groundOffset = dimensions.Dim1.y;
				else if (fabs(rotation.y) > 70.0f)
					groundOffset = dimensions.Dim1.x;

				const Vector3 position = SpoonerCamera::camera.RaycastForCoord(
					Vector2(0.0f, 0.0f), entity, 90.0f, 15.0f + dimensions.Dim2.y);
				entity.SetPosition(SnapPos(position + Vector3(0.0f, 0.0f, groundOffset)));
			}

			void EndNativeEntityDrag()
			{
				if (!nativeCursor.dragging) return;

				GTAentity entity(nativeCursor.draggedEntityHandle);
				if (entity.Exists())
				{
					entity.RequestControl();
					entity.SetIsCollisionEnabled(nativeCursor.draggedEntityHadCollision);
				}

				nativeCursor.dragging = false;
				nativeCursor.draggedEntityHandle = 0;
				nativeCursor.draggedEntityHadCollision = true;
			}

			void UpdateNativeCursorState(DWORD now, const GTAentity& activeEntity)
			{
				const float deltaSeconds = nativeCursor.lastUpdateTime == 0
					? 0.0f
					: static_cast<float>(now - nativeCursor.lastUpdateTime) / 1000.0f;
				nativeCursor.lastUpdateTime = now;

				const bool visible = ShouldDrawNativeCursor();
				const bool hasEntity = visible && activeEntity.Exists();
				nativeCursor.visibilityOpacity = AnimateNativeCursorOpacity(nativeCursor.visibilityOpacity, visible ? 1.0f : 0.0f, deltaSeconds, 0.20f);
				nativeCursor.hoverOpacity = AnimateNativeCursorOpacity(nativeCursor.hoverOpacity, hasEntity ? 1.0f : 0.0f, deltaSeconds, 0.16f);
				nativeCursor.textOpacity = AnimateNativeCursorOpacity(nativeCursor.textOpacity, hasEntity ? 1.0f : 0.0f, deltaSeconds, hasEntity ? 0.16f : 0.06f);

				const int activeHandle = hasEntity ? activeEntity.GetHandle() : 0;
				if (activeHandle != nativeCursor.hoveredEntityHandle)
				{
					nativeCursor.hoveredEntityHandle = activeHandle;
					nativeCursor.hoveredEntityName = hasEntity
						? GetNativeCursorEntityName(activeEntity)
						: std::string();
				}
			}

			void HandleNativeCursorInput(const GTAentity& hoveredEntity)
			{
				if (!ShouldDrawNativeCursor())
				{
					EndNativeEntityDrag();
					return;
				}

				GTAentity activeEntity = nativeCursor.dragging
					? GTAentity(nativeCursor.draggedEntityHandle)
					: hoveredEntity;

				if (IS_DISABLED_CONTROL_JUST_PRESSED(2, INPUT_CURSOR_CANCEL) && activeEntity.Exists())
				{
					if (!nativeCursor.dragging)
						SetAsSelectedEntity(activeEntity);
					OpenMenu(SUB::SPOONER_SELECTEDENTITYOPS);
					return;
				}

				if (!nativeCursor.dragging && hoveredEntity.Exists() && IS_DISABLED_CONTROL_PRESSED(2, INPUT_CURSOR_ACCEPT))
					BeginNativeEntityDrag(hoveredEntity);

				if (nativeCursor.dragging)
				{
					if (IS_DISABLED_CONTROL_PRESSED(2, INPUT_CURSOR_ACCEPT))
						UpdateNativeEntityDrag();
					else
						EndNativeEntityDrag();
				}
			}

			void DrawNativeCursor()
			{
				if (nativeCursor.visibilityOpacity <= 0.0f) return;
				if (!HAS_STREAMED_TEXTURE_DICT_LOADED("mpinventory"))
				{
					REQUEST_STREAMED_TEXTURE_DICT("mpinventory", false);
					return;
				}

				const int whiteAlpha = static_cast<int>(255.0f * nativeCursor.visibilityOpacity * (1.0f - nativeCursor.hoverOpacity));
				const int greenAlpha = static_cast<int>(255.0f * nativeCursor.visibilityOpacity * nativeCursor.hoverOpacity);
				int screenWidth = 0, screenHeight = 0;
				GET_SCREEN_RESOLUTION(&screenWidth, &screenHeight);
				const float aspectRatio = screenHeight > 0 ? static_cast<float>(screenWidth) / screenHeight : 1.0f;
				const float dotWidth = nativeCursorDotSize / aspectRatio;
				if (whiteAlpha > 0)
					DRAW_SPRITE("mpinventory", "in_world_circle", 0.5f, 0.5f, dotWidth, nativeCursorDotSize, 0.0f, 255, 255, 255, whiteAlpha, false, 0);
				if (greenAlpha > 0)
					DRAW_SPRITE("mpinventory", "in_world_circle", 0.5f, 0.5f, dotWidth, nativeCursorDotSize, 0.0f, 0, 255, 0, greenAlpha, false, 0);

				if (nativeCursor.hoveredEntityName.empty() || nativeCursor.textOpacity <= 0.0f)
					return;

				const UINT8 textAlpha = static_cast<UINT8>(255.0f * nativeCursor.visibilityOpacity * nativeCursor.textOpacity);
				Game::Print::SetupDraw(GTAfont::Arial, Vector2(0.35f, 0.35f), true, false, true, RGBA(255, 255, 255, textAlpha));
				Game::Print::drawstring(nativeCursor.hoveredEntityName, 0.5f, nativeCursorTextY);
				Game::Print::SetupDraw(GTAfont::Arial, Vector2(0.22f, 0.22f), true, false, true, RGBA(195, 195, 195, static_cast<UINT8>(textAlpha * 0.75f)));
				Game::Print::drawstring("Right click to manage this entity", 0.5f, nativeCursorInstructionY);
			}

			void TickNativeCursor()
			{
				if (!ShouldDrawNativeCursor())
				{
					HandleNativeCursorInput(GTAentity());
					UpdateNativeCursorState(GetTickCount(), GTAentity());
					DrawNativeCursor();
					return;
				}

				const GTAentity hoveredEntity = nativeCursor.dragging
					? GTAentity(nativeCursor.draggedEntityHandle)
					: GetNativeCursorEntity();
				HandleNativeCursorInput(hoveredEntity);
				const GTAentity activeEntity = nativeCursor.dragging
					? GTAentity(nativeCursor.draggedEntityHandle)
					: hoveredEntity;
				UpdateNativeCursorState(GetTickCount(), activeEntity);
				DrawNativeCursor();
			}
		}

		BYTE bindsKeyboard = VirtualKey::F9;
		std::pair<UINT16, UINT16> bindsGamepad = { INPUT_FRONTEND_RB, INPUT_FRONTEND_RIGHT };

		bool bEnabled = false;
		EditingState editingState;
		SpoonerStats GetSpoonerStats()
		{
			SpoonerStats stats = { 0, 0, 0, 0 };
			stats.totalNumEntities = (UINT)Databases::EntityDb.size();
			for (auto& spoonerEntity : Databases::EntityDb)
			{
				switch (spoonerEntity.type)
				{
				case EntityType::PROP: stats.totalNumProps++; break;
				case EntityType::PED: stats.totalNumPeds++; break;
				case EntityType::VEHICLE: stats.totalNumVehicles++; break;
				}
			}
			return stats;
		}

		static bool IsHotkeyPressed()
		{
			UINT8 index1 = bindsGamepad.first < 50 ? 0 : 2;
			UINT8 index2 = bindsGamepad.second < 50 ? 0 : 2;
			return Menu::usingControllerInput
				? IS_DISABLED_CONTROL_PRESSED(index1, bindsGamepad.first) && IS_DISABLED_CONTROL_JUST_PRESSED(index2, bindsGamepad.second)
				: IsKeyJustUp(bindsKeyboard);
		}

		static bool IsCursorKeyPressed()
		{
			if (!bEnabled) return false;
			if (Menu::usingControllerInput) return false;
			return IsKeyJustUp(VirtualKey::Tab);
		}

		Vector3 SnapPos(Vector3 pos)
		{
			if (Settings::bGridSnapEnabled && Settings::gridSnapSize > 0.0f)
			{
				float g = Settings::gridSnapSize;
				pos.x = round(pos.x / g) * g;
				pos.y = round(pos.y / g) * g;
				if (!Settings::bSnapToGround)
					pos.z = round(pos.z / g) * g;
			}
			if (Settings::bSnapToGround)
			{
				float groundZ;
				if (GET_GROUND_Z_FOR_3D_COORD(pos.x, pos.y, pos.z + 0.1f, &groundZ, false, false))
					pos.z = groundZ;
			}
			return pos;
		}
		Vector3 SnapRot(Vector3 rot)
		{
			if (Settings::bGridSnapEnabled && Settings::rotationSnapDegrees > 0.0f)
			{
				float r = Settings::rotationSnapDegrees;
				rot.x = round(rot.x / r) * r;
				rot.y = round(rot.y / r) * r;
				rot.z = round(rot.z / r) * r;
			}
			return rot;
		}

		void DrawSnappingGrid()
		{
			float gridSize = Settings::gridSnapSize;

			Vector3 origin = selectedEntity.handle.GetPosition();
			origin.x = round(origin.x / gridSize) * gridSize;
			origin.y = round(origin.y / gridSize) * gridSize;

			float z = round(origin.z / gridSize) * gridSize;
			const int cells = 10; // number of cells to draw in each direction (i.e setting this to 10 will draw a 20x20 grid)
			const RGBA color(255, 255, 255, 110);

			for (int i = -cells; i <= cells; i++)
			{
				float x = origin.x + i * gridSize;
				Vector3 start(x, origin.y - cells * gridSize, z);
				Vector3 end(x, origin.y + cells * gridSize, z);
				World::DrawLine(start, end, color);
			}

			for (int i = -cells; i <= cells; i++)
			{
				float y = origin.y + i * gridSize;
				Vector3 start(origin.x - cells * gridSize, y, z);
				Vector3 end(origin.x + cells * gridSize, y, z);
				World::DrawLine(start, end, color);
			}
		}

		ModelPreviewInfoStructure modelPreviewInfo = { EntityType::ALL, 0, 0, 0,{} };
		float previewYawOffset = 0.0f;

		void UpdatePreviewRotation()
		{
			if (modelPreviewInfo.entity.Exists() && Menu::activeSubmenu != SUB::CLOSED)
			{
				Menu::add_IB(INPUT_FRONTEND_RB, "");
				Menu::add_IB(INPUT_FRONTEND_LB, "Rotate Preview");

				bool lbPressed = IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_LB);
				bool rbPressed = IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_RB);
				bool dpadPressed = IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_LEFT) ||
					IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_RIGHT) ||
					IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_UP) ||
					IS_DISABLED_CONTROL_PRESSED(2, INPUT_FRONTEND_DOWN);

				if (!dpadPressed)
				{
					if (lbPressed && !rbPressed) previewYawOffset -= 2.0f;
					if (rbPressed && !lbPressed) previewYawOffset += 2.0f;
					if (previewYawOffset > 360.0f || previewYawOffset < -360.0f) previewYawOffset = fmod(previewYawOffset, 360.0f);
				}
			}
		}

		void SpawnModelPreview()
		{
			bool bOnTheLine = NETWORK_IS_IN_SESSION() != 0;
			auto& info = modelPreviewInfo;
			if (info.entityType == EntityType::ALL)
			{
				if (info.entity != 0)
				{
					if (bOnTheLine)
					{
						info.previousEntities.insert(info.entity);
					}
					else
					{
						info.entity.Delete(true);
						info.entity = 0;
					}
				}
				if (info.model.hash != 0)
				{
					if (info.model.IsLoaded())
						info.model.Unload();
					info.model = 0;
				}
				if (info.previousModel.hash != 0)
				{
					if (info.previousModel.IsLoaded())
						info.previousModel.Unload();
					info.previousModel = 0;
				}
			}
			else if (info.model != info.previousModel)
			{
				previewYawOffset = 0.0f;
				if (bOnTheLine)
				{
					info.previousEntities.insert(info.entity);
				}
				else
				{
					info.entity.Delete(true);
					info.entity = 0;
				}
				if (info.previousModel.IsLoaded())
					info.previousModel.Unload();
				info.previousModel = info.model;
				if (info.model.IsInCdImage())
					info.model.Load();
			}
			else
			{
				if (info.entity.Exists())
				{
					const ModelDimensions& dimensions = info.model.Dimensions();

					Vector3 spawnRot(0, 0, SpoonerCamera::camera.GetRotation().z + previewYawOffset);

					const Vector3& geSep = info.entity.GetPosition();
					//auto& geGroundRay = RaycastResult::Raycast(geSep, Vector3::WorldDown(), max(max(dimensions.Dim1.x, dimensions.Dim2.x), max(max(dimensions.Dim1.y, dimensions.Dim2.y), max(dimensions.Dim1.z, dimensions.Dim2.z))) + 2.0f, IntersectOptions::Everything, info.entity);
					float geGroundZ = dimensions.Dim1.z;
					//if (geGroundRay.DidHitAnything()){
					//float oldYaw = spawnRot.z;
					//geGroundZ = geGroundRay.HitCoords().DistanceTo(geSep);
					//Vector3 spawnRot;
					//spawnRot = Vector3::DirectionToRotation(geGroundRay.SurfaceNormal());
					//spawnRot.x += 90.0f;
					//spawnRot.z = oldYaw;
					//}
					if (abs(spawnRot.x) > 150.0f || abs(spawnRot.y) > 150.0f) geGroundZ = dimensions.Dim2.z;
					else if (abs(spawnRot.x) > 70.0f && abs(spawnRot.y) > 70.0f) geGroundZ = (dimensions.Dim1.y + dimensions.Dim1.x) / 2;
					else if (abs(spawnRot.x) > 70.0f) geGroundZ = dimensions.Dim1.y;
					else if (abs(spawnRot.y) > 70.0f) geGroundZ = dimensions.Dim1.x;
					Vector3 spawnPos(SpoonerCamera::camera.RaycastForCoord(Vector2(0.0f, 0.0f), info.entity, 120.0f, 23.0f + dimensions.Dim2.y) + Vector3(0, 0, geGroundZ));

					spawnPos = SnapPos(spawnPos);
					if (Settings::rotationSnapDegrees > 0.0f)
					{
						float r = Settings::rotationSnapDegrees;
						spawnRot.z = round(spawnRot.z / r) * r;
					}

					if (bOnTheLine)
						info.entity.RequestControlOnce();
					info.entity.SetRotation(spawnRot);
					info.entity.SetPosition(spawnPos);
					EntityManagement::ShowBoxAroundEntity(info.entity, false, RGBA::AllWhite());
				}
				else
				{
					if (info.model.IsLoaded())
					{
						switch (info.entityType)
						{
						case EntityType::PROP:
							info.entity = World::CreateProp(info.model, Vector3(), Vector3(), false, false);
							break;
						case EntityType::PED:
							info.entity = World::CreatePed(info.model, Vector3(), Vector3(), false);
							break;
						case EntityType::VEHICLE:
							info.entity = World::CreateVehicle(info.model, Vector3(), Vector3(), false);
							break;
						}
						info.entity.FreezePosition(true);
						info.entity.SetIsCollisionEnabled(false);
						info.entity.SetAlpha(120);
					}
				}
			}

			info.entityType = EntityType::ALL;

			for (auto it = info.previousEntities.begin(); it != info.previousEntities.end();)
			{
				GTAentity e = *it;
				if (e.RequestControlOnce())
				{
					if (e == info.entity)
						info.entity = 0;
					e.Delete(true);
					it = info.previousEntities.erase(it);
				}
				else ++it;
			}
		}

		void ResetSelectedEntity()
		{
			selectedEntity.handle = 0;
		}
		bool GetEntityPtr(GTAentity& inEntity, SpoonerEntity*& outEntity)
		{
			outEntity = new SpoonerEntity;

			outEntity->handle = inEntity;
			outEntity->type = (EntityType)inEntity.Type();
			const Model& outEntityModel = inEntity.Model();
			outEntity->hashName = outEntity->type == EntityType::PROP ? get_prop_model_label(outEntityModel)
				: (outEntity->type == EntityType::PED ? GetPedModelLabel(outEntityModel, true)
					: get_vehicle_model_label(outEntityModel, true));
			if (outEntity->hashName.length() == 0) outEntity->hashName = IntToHexString(outEntityModel.hash, true);
			outEntity->dynamic = !outEntity->handle.IsPositionFrozen();//outEntity->type == EntityType::PED || outEntity->type == EntityType::VEHICLE;
			//outEntity->lastAnimations.clear();
			//outEntity->currentScenario.clear();
			outEntity->isStill = false;

			auto idInDb = EntityManagement::GetEntityIndexInDb(*outEntity);
			if (idInDb >= 0)
			{
				delete outEntity;
				outEntity = &Databases::EntityDb[idInDb];
				return true; // Is in db
			}
			else
			{
				return false; // Is not in db
			}
		}
		SpoonerEntity GetEntityPtrValue(GTAentity& entity)
		{
			SpoonerEntity* eifoc = nullptr;
			bool isAlreadyInDb = SpoonerMode::GetEntityPtr(entity, eifoc);
			SpoonerEntity toReturn = *eifoc;
			if (!isAlreadyInDb)
				delete eifoc;
			return toReturn;
		}
		void SetAsSelectedEntity(GTAentity& entity)
		{
			SpoonerEntity* eifoc = nullptr;
			bool isAlreadyInDb = SpoonerMode::GetEntityPtr(entity, eifoc);
			selectedEntity = *eifoc;
			selectedEntity.handle.RequestControl();
			if (!isAlreadyInDb)
				delete eifoc;
		}

		void OpenMenu(int submenu, int selectedOption)
		{
			std::fill(std::begin(Menu::submenuHistory), std::end(Menu::submenuHistory), 0);
			std::fill(std::begin(Menu::optionSelectionHistory), std::end(Menu::optionSelectionHistory), 0);
			Menu::submenuHistory[0] = SUB::MAINMENU;
			Menu::optionSelectionHistory[0] = 1;
			Menu::menuHistoryIndex = 0;
			Menu::NewSetMenu(submenu);
			if (selectedOption < 1) selectedOption = 1;
			Menu::selectedOptionIndex = selectedOption;
			Menu::selectedOptionWithBreaks = selectedOption;
			*Menu::activeOptionIndex = selectedOption;
		}

		static void HandleTickHotkeys()
		{
			if (IsHotkeyPressed())
				Toggle();

			if (!IsCursorKeyPressed()) return;

			Settings::bCursorMode = !Settings::bCursorMode;
			ImGuiSpooner::SetCursorModeEnabled(Settings::bCursorMode);
			if (!Settings::bCursorMode)
				editingState.mode = eEditMode::Disabled;
		}

		static void DrawSpoonerOverlays()
		{
			if (Settings::bShowBoxAroundSelectedEntity)
				EntityManagement::ShowBoxAroundEntity(selectedEntity.handle);

			if (Settings::bDrawGrid && Settings::bGridSnapEnabled && bEnabled && selectedEntity.handle.Exists())
				DrawSnappingGrid();
		}

		static void UpdateCursorEditingState()
		{
			if (!bEnabled || !Settings::bCursorMode || Menu::activeSubmenu != SUB::CLOSED) return;
			if (!selectedEntity.handle.Exists()) return;

			if (editingState.mode == eEditMode::Gizmo && IsKeyJustUp(VirtualKey::R))
			{
				switch (editingState.transformMode)
				{
				case eTransformMode::Position: editingState.transformMode = eTransformMode::Rotation; break;
				case eTransformMode::Rotation: editingState.transformMode = eTransformMode::Scale; break;
				case eTransformMode::Scale: editingState.transformMode = eTransformMode::Position; break;
				}
			}

			if (editingState.mode == eEditMode::Gizmo && IsKeyJustUp(VirtualKey::L))
				editingState.localSpace = !editingState.localSpace;

			if (IsKeyJustUp(VirtualKey::Menu))
			{
				selectedEntity = EntityManagement::CopyEntity(
					selectedEntity,
					EntityManagement::GetEntityIndexInDb(selectedEntity) >= 0,
					true,
					Submenus::_copyEntTexterValue);
				Game::Print::ShowNotification("Entity copied.", 2.5f);
			}

			DrawEditingHUD();
		}

		static void TickEntityTasks()
		{
			for (auto& entity : Databases::EntityDb)
			{
				if (entity.handle.Exists())
					entity.taskSequence.Tick(reinterpret_cast<void*>(&entity));
			}
		}

		static void DrawSpoonerWorldItems()
		{
			if (!Databases::MarkerDb.empty())
				MarkerManagement::DrawAll();
			if (!Databases::LightDb.empty())
				LightManagement::DrawAll();
		}

		static void ApplyEntityScales()
		{
			auto applyScale = [](const Submenus::EntityScaleState& state)
			{
				if (state.handle == 0) return;
				GTAentity(state.handle).SetScale(state.scale);
			};

			applyScale(Submenus::_vehScale);
			applyScale(Submenus::_pedScale);
			applyScale(Submenus::_objScale);
		}

		void Tick()
		{
			HandleTickHotkeys();
			ImGuiSpooner::Tick();
			UpdatePreviewRotation();
			SpoonerCamera::Tick();
			TickNativeCursor();
			UpdateCursorEditingState();
			DrawSpoonerOverlays();
			TickEntityTasks();
			DrawSpoonerWorldItems();
			ApplyEntityScales();
		}

		void TurnOn()
		{
			if (!menuHasNotOpened)
			{
				SpoonerMode::bEnabled = true;
				sub::Spooner::ImGuiSpooner::SetVisible(true);
				if (Menu::activeSubmenu != SUB::CLOSED)
					Game::Print::PrintBottomLeft("~b~Note:~s~ Spooner Mode instructions only appear when Menyoo is closed.");
			}
			else
			{
				Game::Print::ShowNotification("~r~Error:", "Menu not opened yet.");
			}
		}
		void TurnOff()
		{
			EndNativeEntityDrag();
			nativeCursor = NativeCursorState{};
			SpoonerMode::bEnabled = false;
			sub::Spooner::ImGuiSpooner::SetVisible(false);
			Settings::bCursorMode = false;
			if (Menu::activeSubmenu != SUB::CLOSED)
				Menu::SetSub_closed();
			SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
			auto& info = modelPreviewInfo;
			for (auto it = info.previousEntities.begin(); it != info.previousEntities.end();)
			{
				GTAentity e = *it;
				e.RequestControl(600);
				if (e != info.entity)
					e.Delete(true);
				++it;
			}
			info.previousEntities.clear();
			if (info.entity != 0)
			{
				info.entityType = EntityType::ALL;
				SpoonerMode::SpawnModelPreview();
			}
		}
		void ProcessKeyboardManipulation(Vector3& position, Vector3& rotation)
		{
			if (!bEnabled) return;

			float& precision = editingState.transformMode == eTransformMode::Position ? editingState.precisionPos
			                 : editingState.transformMode == eTransformMode::Rotation ? editingState.precisionRot
			                 : editingState.precisionScale;

			static DWORD lastSensitivityChange = 0;
			if (IsKeyJustUp(VirtualKey::OEMPlus) && GetTickCount() - lastSensitivityChange > 200)
			{
				if (precision < 10.0f) precision *= 10;
				lastSensitivityChange = GetTickCount();
				Game::Print::PrintBottomCentre("Sensitivity: ~b~" + std::to_string(precision), 3000);
			}
			if (IsKeyJustUp(VirtualKey::OEMMinus) && GetTickCount() - lastSensitivityChange > 200)
			{
				if (precision > 0.0001f) precision /= 10;
				lastSensitivityChange = GetTickCount();
				Game::Print::PrintBottomCentre("Sensitivity: ~b~" + std::to_string(precision), 3000);
			}

			float step = precision;
			// if grid snap is enabled, override precision with the snap amount for the current transform mode
			if (Settings::bGridSnapEnabled)
			{
				float snapAmount = editingState.transformMode == eTransformMode::Rotation
					? Settings::rotationSnapDegrees
					: Settings::gridSnapSize;
				if (snapAmount > 0.0f) step = snapAmount;
			}

			auto& target = editingState.transformMode == eTransformMode::Rotation ? rotation : position;
			if (IsKeyDown(VirtualKey::W)) target.x += step;
			if (IsKeyDown(VirtualKey::S)) target.x -= step;
			if (IsKeyDown(VirtualKey::A)) target.y += step;
			if (IsKeyDown(VirtualKey::D)) target.y -= step;
			if (IsKeyDown(VirtualKey::E)) target.z += step;
			if (IsKeyDown(VirtualKey::Q)) target.z -= step;

			if (editingState.transformMode == eTransformMode::Rotation)
				rotation = SnapRot(rotation);
			else
				position = SnapPos(position);
		}

		void DrawEditingHUD()
		{
			constexpr float HUD_LINE_HEIGHT = 0.025f;
			const Vector2 HUD_FONT_SIZE(0.35f, 0.35f);
			constexpr float hudX = 0.02f;
			float hudY = 0.8f;

			auto drawText = [&](const std::string& text, RGBA colour = {255, 255, 255, 255})
			{
				Game::Print::SetupDraw(GTAfont::Arial, HUD_FONT_SIZE, false, false, true, colour);
				Game::Print::drawstring(text, hudX, hudY);
				hudY += HUD_LINE_HEIGHT;
			};

			if (!bEnabled)
			{
				drawText("~r~Entity manipulation requires the Spooner Camera.");
				drawText("~b~Press F9:~w~ Enable Spooner Mode.");
				return;
			}

			if (editingState.mode == eEditMode::Disabled)
			{
				drawText("~r~Entity manipulation DISABLED.");
				drawText("~b~Press B:~w~ Enable keyboard controls or gizmo editing mode.");
			}
			else if (editingState.mode == eEditMode::Keyboard)
			{
				if (editingState.transformMode == eTransformMode::Rotation)
				{
					drawText("~y~Rotation Mode:");
					drawText("~b~W/S: ~w~Pitch+ / Pitch-");
					drawText("~b~A/D: ~w~Yaw+ / Yaw-");
					drawText("~b~E/Q: ~w~Roll+ / Roll-");
					drawText("~b~=/-: ~w~+/- Sensitivity");
					drawText("~b~R: ~w~Edit position");
				}
				else
				{
					drawText("~y~Position Mode:");
					drawText("~b~W/S: ~w~X+ / X-");
					drawText("~b~A/D: ~w~Y+ / Y-");
					drawText("~b~E/Q: ~w~Z+ / Z-");
					drawText("~b~=/-: ~w~+/- Sensitivity");
					drawText("~b~R: ~w~Edit rotation");
				}
				drawText("~b~ALT: ~w~Copy entity");
				drawText("~b~B: ~w~Switch to gizmo / disable controls.");
			}
			else if (editingState.mode == eEditMode::Gizmo)
			{
				std::string modeName;
				switch (editingState.transformMode)
				{
					case eTransformMode::Rotation: modeName = "Rotation"; break;
					case eTransformMode::Scale:    modeName = "Scale";    break;
					default:                             modeName = "Position"; break;
				}
				drawText("~y~Gizmo Mode ~s~(" + modeName + " Mode):");
				drawText("~b~Left Click:~w~ Grab axis handle");
				drawText("~b~R:~w~ Cycle mode");
				drawText(Settings::bCursorMode ? "~b~Tab:~w~ Disable cursor mode" : "~b~Tab:~w~ Enable cursor mode");
				drawText(editingState.localSpace ? "~b~L:~w~ Edit in world space" : "~b~L:~w~ Edit in local space");
				drawText("~b~ALT:~w~ Copy entity");
				drawText(Settings::bCursorMode ? "~b~Click empty space:~w~ Deselect entity" : "~b~B:~w~ Disable gizmo mode");
			}
		}

		void UpdateEntityEditingState(Vector3& position, Vector3& rotation)
		{
			// toggling between Disabled / Keyboard / Gizmo modes
			static bool lastBToggle = false;
			bool currentBToggle = IsKeyJustUp(VirtualKey::B);
			if (currentBToggle && !lastBToggle)
			{
				switch (editingState.mode)
				{
					case eEditMode::Disabled:
						editingState.mode = eEditMode::Keyboard;
						break;
					case eEditMode::Keyboard:
						editingState.mode = eEditMode::Gizmo;
						break;
					case eEditMode::Gizmo:
						editingState.mode = eEditMode::Disabled;
						break;
					}
			}
			lastBToggle = currentBToggle;

			// toggling between transform modes
			static bool lastRToggle = false;
			bool currentRToggle = IsKeyJustUp(VirtualKey::R);
			if (currentRToggle && !lastRToggle)
			{
				if (editingState.mode != eEditMode::Disabled)
				{
					// In keyboard mode, R just toggles between position and rotation editing (scale is not supported in keyboard mode)
					static const eTransformMode table[2][3] = {
						// Position, Rotation, Scale
						{ eTransformMode::Rotation, eTransformMode::Position, eTransformMode::Position }, // Keyboard editing mode (scale is not supported, it just redirects to position)
						{ eTransformMode::Rotation, eTransformMode::Scale,    eTransformMode::Position }  // Gizmo editing mode
					};
					editingState.transformMode = table[(int)editingState.mode - 1][(int)editingState.transformMode];
				}
			}
			lastRToggle = currentRToggle;

			// toggling world / local space editing
			if (editingState.mode != eEditMode::Disabled && IsKeyJustUp(VirtualKey::L))
			{
				editingState.localSpace = !editingState.localSpace;
			}

			// make a quick copy of an entity by clicking ALT in editing modes
			if (editingState.mode != eEditMode::Disabled && IsKeyJustUp(VirtualKey::Menu))
			{
				if (selectedEntity.handle.Exists())
				{
					const SpoonerEntity& copiedEntity = EntityManagement::CopyEntity(selectedEntity, EntityManagement::GetEntityIndexInDb(selectedEntity) >= 0, true, Submenus::_copyEntTexterValue);
					selectedEntity = copiedEntity;
					Game::Print::ShowNotification("Entity copied.", 2.5f);
				}
			}

			if (editingState.mode == eEditMode::Keyboard)
			{
				// keyboard edit mode doesn't support scaling
				if (editingState.transformMode == eTransformMode::Scale)
					editingState.transformMode = eTransformMode::Position;
				ProcessKeyboardManipulation(position, rotation);
			}

			DrawEditingHUD();
		}

		void Toggle()
		{
			SpoonerMode::bEnabled ? SpoonerMode::TurnOff() : SpoonerMode::TurnOn();
		}
	}

}
