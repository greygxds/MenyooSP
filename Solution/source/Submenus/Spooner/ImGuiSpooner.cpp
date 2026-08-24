#include "ImGuiSpooner.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"
#include <cfloat>
#include "D3D11Hook.h"

#include <d3d11.h>
#include <Windows.h>
#include <atomic>
#include <mutex>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <optional>
#include <vector>
#include <pugixml/src/pugixml.hpp>

#include "SpoonerEntity.h"
#include "SpoonerMode.h"
#include "SpoonerSettings.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\Model.h"
#include "..\..\Scripting\Camera.h"
#include "..\..\Scripting\World.h"
#include "..\..\Scripting\Game.h"
#include "..\..\Util\GTAmath.h"
#include "..\..\Natives\natives.h"
#include "..\..\Natives\natives2.h"
#include "..\..\Menu\Menu.h"
#include "..\..\Menu\Routine.h"
#include "..\..\Util\ExePath.h"
#include "Submenus.h"
#include "EntityManagement.h"
#include "Databases.h"


namespace sub::Spooner::ImGuiSpooner
{
// ═══════════════════════════════════════════════════════════════════
//  Shared State
// ═══════════════════════════════════════════════════════════════════

	static std::mutex g_Mutex;
	SharedState g_Shared;

	void SetCommand(SharedState& state, CursorCommand command, int intPayload, int dbPayload, float floatPayload, FavouriteSpawnPayload spawnPayload)
	{
		auto& queue = state.cmds.queue;
		if (queue.size() >= 16) return;
		queue.push_back(QueuedCommand{command, intPayload, dbPayload, floatPayload, spawnPayload, state.cursorScreenX, state.cursorScreenY});
	}

	static std::atomic<bool> g_Visible{ false };
	static std::atomic<bool> g_ShuttingDown{ false };
	std::atomic<bool> g_ContextMenuReady{ false };
	std::atomic<bool> g_EmptySpaceMenuReady{ false };
	static bool g_ImGuiInitialized = false;

	// ── Async spawn state ──
	struct PendingSpawn
	{
		Hash modelHash = 0;
		uint8_t category = 0;
		std::string name;
		DWORD startTime = 0;
		bool active = false;
	};
	static PendingSpawn g_PendingSpawn;

// ═══════════════════════════════════════════════════════════════════
//  Gizmo Math
// ═══════════════════════════════════════════════════════════════════

	static void BuildTransformMatrix(const Vector3& pos, const Vector3& rot, const Vector3& scale, float* matrix)
	{
		constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;
		float pitch = rot.x * DEG2RAD;
		float roll  = rot.y * DEG2RAD;
		float yaw   = rot.z * DEG2RAD;
		float cp = cosf(pitch), sp = sinf(pitch);
		float cr = cosf(roll),  sr = sinf(roll);
		float cy = cosf(yaw),   sy = sinf(yaw);

		float col0[3] = { cy*cr - sy*sp*sr, sy*cr + cy*sp*sr, -cp*sr };
		float col1[3] = { -sy*cp, cy*cp, sp };
		float col2[3] = { cy*sr + sy*sp*cr, sy*sr - cy*sp*cr, cp*cr };

		matrix[0]  = col0[0] * scale.x;
		matrix[1]  = col0[1] * scale.x;
		matrix[2]  = col0[2] * scale.x;
		matrix[3]  = 0.0f;

		matrix[4]  = col1[0] * scale.y;
		matrix[5]  = col1[1] * scale.y;
		matrix[6]  = col1[2] * scale.y;
		matrix[7]  = 0.0f;

		matrix[8]  = col2[0] * scale.z;
		matrix[9]  = col2[1] * scale.z;
		matrix[10] = col2[2] * scale.z;
		matrix[11] = 0.0f;

		matrix[12] = pos.x;
		matrix[13] = pos.y;
		matrix[14] = pos.z;
		matrix[15] = 1.0f;
	}

	static void DecomposeTransformMatrix(const float* matrix, Vector3& pos, Vector3& rot, Vector3& scale)
	{
		pos.x = matrix[12];
		pos.y = matrix[13];
		pos.z = matrix[14];

		scale.x = sqrtf(matrix[0]*matrix[0] + matrix[1]*matrix[1] + matrix[2]*matrix[2]);
		scale.y = sqrtf(matrix[4]*matrix[4] + matrix[5]*matrix[5] + matrix[6]*matrix[6]);
		scale.z = sqrtf(matrix[8]*matrix[8] + matrix[9]*matrix[9] + matrix[10]*matrix[10]);

		float invSx = (scale.x > 1e-8f) ? 1.0f / scale.x : 0.0f;
		float invSy = (scale.y > 1e-8f) ? 1.0f / scale.y : 0.0f;
		float invSz = (scale.z > 1e-8f) ? 1.0f / scale.z : 0.0f;

		float col0[3] = { matrix[0] * invSx, matrix[1] * invSx, matrix[2] * invSx };
		float col1[3] = { matrix[4] * invSy, matrix[5] * invSy, matrix[6] * invSy };
		float col2[3] = { matrix[8] * invSz, matrix[9] * invSz, matrix[10] * invSz };

		constexpr float RAD2DEG = 180.0f / 3.14159265358979323846f;

		float sp = col1[2];
		if (sp > 1.0f) sp = 1.0f;
		if (sp < -1.0f) sp = -1.0f;
		float pitch = asinf(sp);
		float cp = cosf(pitch);

		float yaw, roll;
		if (fabsf(cp) > 1e-5f)
		{
			yaw  = atan2f(-col1[0], col1[1]);
			roll = atan2f(-col0[2], col2[2]);
		}
		else
		{
			yaw  = atan2f(col0[1], col0[0]);
			roll = 0.0f;
		}

		rot.x = pitch * RAD2DEG;
		rot.y = roll  * RAD2DEG;
		rot.z = yaw   * RAD2DEG;
	}

	static void BuildCameraMatricesFromCache(const Vector3& camCoord, const Vector3& camRot,
		float camFov, float screenW, float screenH,
		float* outView, float* outProj)
	{
		constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;

		float h = camRot.z * DEG2RAD;
		float p = camRot.x * DEG2RAD;
		float r = camRot.y * DEG2RAD;

		float cosP = cosf(p), sinP = sinf(p);
		float cosH = cosf(h), sinH = sinf(h);
		float cosR = cosf(r), sinR = sinf(r);

		float rightX = cosH * cosR - sinH * sinP * sinR;
		float rightY = sinH * cosR + cosH * sinP * sinR;
		float rightZ = -cosP * sinR;

		float fwdX = -sinH * cosP;
		float fwdY = cosH * cosP;
		float fwdZ = sinP;

		float upX = cosH * sinR + sinH * sinP * cosR;
		float upY = sinH * sinR - cosH * sinP * cosR;
		float upZ = cosP * cosR;

		float eyeX = camCoord.x, eyeY = camCoord.y, eyeZ = camCoord.z;

		outView[0] = rightX;  outView[4] = rightY;  outView[8] = rightZ;
		outView[12] = -(rightX * eyeX + rightY * eyeY + rightZ * eyeZ);
		outView[1] = upX;     outView[5] = upY;     outView[9] = upZ;
		outView[13] = -(upX * eyeX + upY * eyeY + upZ * eyeZ);
		outView[2] = -fwdX;   outView[6] = -fwdY;   outView[10] = -fwdZ;
		outView[14] = (fwdX * eyeX + fwdY * eyeY + fwdZ * eyeZ);
		outView[3] = 0;       outView[7] = 0;       outView[11] = 0;
		outView[15] = 1;

		float aspect = screenH > 0 ? screenW / screenH : 16.0f / 9.0f;
		float fovRad = camFov * DEG2RAD;
		float f = 1.0f / tanf(fovRad * 0.5f);
		float nearZ = 0.1f;
		float farZ = 10000.0f;

		memset(outProj, 0, sizeof(float) * 16);
		outProj[0] = f / aspect;
		outProj[5] = f;
		outProj[10] = (farZ + nearZ) / (nearZ - farZ);
		outProj[11] = -1.0f;
		outProj[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
	}

	static float UnwrapAngle(float cur, float prev)
	{
		float diff = cur - prev;
		if (diff > 180.0f) cur -= 360.0f;
		if (diff < -180.0f) cur += 360.0f;
		return cur;
	}

	static void Mat4Mul(const float a[16], const float b[16], float out[16])
	{
		for (int row = 0; row < 4; row++)
			for (int col = 0; col < 4; col++) {
				out[row + col * 4] =
					a[row + 0 * 4] * b[0 + col * 4] +
					a[row + 1 * 4] * b[1 + col * 4] +
					a[row + 2 * 4] * b[2 + col * 4] +
					a[row + 3 * 4] * b[3 + col * 4];
			}
	}

	static void Mat4Transpose(const float in[16], float out[16])
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				out[i + j * 4] = in[j + i * 4];
			}
		}
	}

// ═══════════════════════════════════════════════════════════════════
//  Gizmo
// ═══════════════════════════════════════════════════════════════════

	static void RunGizmo_NoLock(SharedState& s)
	{
		s.render.gizmoOver = false;
		s.render.gizmoUsing = false;

		if (!s.cache.entityValid || s.render.editingState.mode != SpoonerMode::eEditMode::Gizmo) return;

		ImGuiIO& io = ImGui::GetIO();

		float viewMat[16], projMat[16];
		BuildCameraMatricesFromCache(s.render.camCoord, s.render.camRot, s.render.camFov, io.DisplaySize.x, io.DisplaySize.y, viewMat, projMat);

		ImGuizmo::BeginFrame();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		ImGuizmo::OPERATION op;
		switch (s.render.editingState.transformMode)
		{
			case SpoonerMode::eTransformMode::Rotation: op = ImGuizmo::ROTATE; break;
			case SpoonerMode::eTransformMode::Scale:    op = ImGuizmo::SCALE;  break;
			default:                                          op = ImGuizmo::TRANSLATE; break;
		}
		ImGuizmo::MODE gizmoMode = s.render.editingState.localSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

		if (op == ImGuizmo::TRANSLATE)
		{
			float matrix[16];
			BuildTransformMatrix(s.cache.position, s.cache.rotation, Vector3(1.0f, 1.0f, 1.0f), matrix);

			float deltaMatrix[16]{};
			float snapMatrix[3] = { s.render.gridSnapSize, s.render.gridSnapSize, s.render.gridSnapSize };

			ImGuizmo::Manipulate(viewMat, projMat, op, gizmoMode, matrix, deltaMatrix,
				(s.render.gridSnapEnabled && s.render.gridSnapSize > 0.0f) ? snapMatrix : nullptr);

			if (ImGuizmo::IsUsing())
			{
				Vector3 newPos(
					s.cache.position.x + deltaMatrix[12],
					s.cache.position.y + deltaMatrix[13],
					s.cache.position.z + deltaMatrix[14]
				);

				if (fabsf(newPos.x - s.cache.position.x) > FLT_EPSILON ||
					fabsf(newPos.y - s.cache.position.y) > FLT_EPSILON ||
					fabsf(newPos.z - s.cache.position.z) > FLT_EPSILON)
				{
					s.pending.positionDirty = true;
					s.pending.positionVal = newPos;
				}
			}
		}
		else if (op == ImGuizmo::ROTATE)
		{
			static float s_DragMatrix[16];
			static float s_LastEuler[3];

			if (!ImGuizmo::IsUsing())
			{
				BuildTransformMatrix(s.cache.position, s.cache.rotation, Vector3(1.0f, 1.0f, 1.0f), s_DragMatrix);
				s_LastEuler[0] = s.cache.rotation.x;
				s_LastEuler[1] = s.cache.rotation.y;
				s_LastEuler[2] = s.cache.rotation.z;
			}

			float oldRot[3] = { s_LastEuler[0], s_LastEuler[1], s_LastEuler[2] };
			float snapMatrix[3] = { s.render.rotationSnapDegrees, s.render.rotationSnapDegrees, s.render.rotationSnapDegrees };
			
			ImGuizmo::Manipulate(viewMat, projMat, op, gizmoMode, s_DragMatrix, nullptr,
				(s.render.gridSnapEnabled && s.render.rotationSnapDegrees > 0.0f) ? snapMatrix : nullptr);

			if (ImGuizmo::IsUsing())
			{
				Vector3 newPos, newRot, newScale;
				DecomposeTransformMatrix(s_DragMatrix, newPos, newRot, newScale);

				newRot.x = UnwrapAngle(newRot.x, oldRot[0]);
				newRot.y = UnwrapAngle(newRot.y, oldRot[1]);
				newRot.z = UnwrapAngle(newRot.z, oldRot[2]);

				if (fabsf(newRot.x - oldRot[0]) > FLT_EPSILON ||
					fabsf(newRot.y - oldRot[1]) > FLT_EPSILON ||
					fabsf(newRot.z - oldRot[2]) > FLT_EPSILON)
				{
					s.pending.rotationDirty = true;
					s.pending.rotationVal = newRot;
				}

				s_LastEuler[0] = newRot.x;
				s_LastEuler[1] = newRot.y;
				s_LastEuler[2] = newRot.z;
			}
		}
		else if (op == ImGuizmo::SCALE)
		{
			static float s_DragMatrix[16];

			if (!ImGuizmo::IsUsing())
				BuildTransformMatrix(s.cache.position, s.cache.rotation, s.cache.scale, s_DragMatrix);

			float deltaMatrix[16] = {0};
			ImGuizmo::Manipulate(viewMat, projMat, ImGuizmo::SCALE, ImGuizmo::LOCAL, s_DragMatrix, deltaMatrix, nullptr);

			if (ImGuizmo::IsUsing())
			{
				Vector3 deltaPos, deltaRot, deltaScale;
				DecomposeTransformMatrix(deltaMatrix, deltaPos, deltaRot, deltaScale);

				Vector3 newScale;
				newScale.x = s.cache.scale.x * deltaScale.x;
				newScale.y = s.cache.scale.y * deltaScale.y;
				newScale.z = s.cache.scale.z * deltaScale.z;

				if (fabsf(newScale.x - s.cache.scale.x) > FLT_EPSILON ||
					fabsf(newScale.y - s.cache.scale.y) > FLT_EPSILON ||
					fabsf(newScale.z - s.cache.scale.z) > FLT_EPSILON)
				{
					s.pending.scaleDirty = true;
					s.pending.scaleVal = newScale;
				}
			}
		}

		s.render.gizmoOver  = ImGuizmo::IsOver();
		s.render.gizmoUsing = ImGuizmo::IsUsing();
	}

// ═══════════════════════════════════════════════════════════════════
//  Context Menu & Theme
// ═══════════════════════════════════════════════════════════════════

	// Adapted from Vulkan-RTX by wpsimon09
	// https://github.com/wpsimon09/Vulkan-RTX/blob/main/Internal/Editor/UIContext/UIContext.cpp#L172
	static void SetColourThemePabloDark()
	{
		ImGuiStyle& style  = ImGui::GetStyle();
		ImVec4*     colors = style.Colors;

		style.WindowRounding    = 8.0f;
		style.ChildRounding     = 8.0f;
		style.FrameRounding     = 6.0f;
		style.PopupRounding     = 6.0f;
		style.ScrollbarRounding = 6.0f;
		style.GrabRounding      = 6.0f;
		style.TabRounding       = 6.0f;

		colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
		colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ChildBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border]                = ImVec4(0.25f, 0.25f, 0.25f, 0.70f);
		colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_FrameBgActive]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_MenuBarBg]             = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_CheckMark]             = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
		colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
		colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
		colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_ButtonHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_ButtonActive]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_Header]                = ImVec4(0.25f, 0.25f, 0.25f, 0.55f);
		colors[ImGuiCol_HeaderHovered]         = ImVec4(0.35f, 0.35f, 0.35f, 0.80f);
		colors[ImGuiCol_HeaderActive]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_Separator]             = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
		colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.45f, 0.45f, 0.45f, 0.78f);
		colors[ImGuiCol_SeparatorActive]       = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_ResizeGrip]            = ImVec4(0.30f, 0.30f, 0.30f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.45f, 0.45f, 0.45f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.50f, 0.50f, 0.50f, 0.95f);
		colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_TabHovered]            = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
		colors[ImGuiCol_TabActive]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TabUnfocused]          = ImVec4(0.10f, 0.10f, 0.10f, 0.97f);
		colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_PlotLines]             = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_PlotHistogram]         = ImVec4(0.80f, 0.65f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.90f, 0.50f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.50f, 0.50f, 0.50f, 0.35f);
		colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 0.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight]          = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

// ═══════════════════════════════════════════════════════════════════
//  D3D11 Render Callback
// ═══════════════════════════════════════════════════════════════════

	static bool ImGui_Init(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		HWND hWnd = D3D11Hook::GetWindowHandle();
		if (!hWnd) return false;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.MouseDrawCursor = false;
		SetColourThemePabloDark();

		if (!ImGui_ImplWin32_Init(hWnd)) { ImGui::DestroyContext(); return false; }
		if (!ImGui_ImplDX11_Init(device, context)) { ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); return false; }
		g_ImGuiInitialized = true;
		return true;
	}

	static void OnRender(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain)
	{
		if (g_ShuttingDown || !g_Visible)
		{
			D3D11Hook::SetMenuVisible(false);
			return;
		}

		if (!g_ImGuiInitialized && !ImGui_Init(device, context))
			return;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		{
			std::lock_guard<std::mutex> lock(g_Mutex);

			bool cursorMode = g_Shared.render.cursorModeEnabled;
			ImGuiIO& io = ImGui::GetIO();

			io.MouseDrawCursor = cursorMode ||
				(g_Shared.render.editingState.mode == SpoonerMode::eEditMode::Gizmo);

			RunGizmo_NoLock(g_Shared);

			if (cursorMode)
				HandleCursorModeClicks(io);

			DrawContextMenu();

			if (cursorMode)
				DrawMenuBarWindow();
		}

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

// ═══════════════════════════════════════════════════════════════════
//  Attachment Gizmo Math
// ═══════════════════════════════════════════════════════════════════

	static Vector3 WorldDeltaToBoneRelative(const Vector3& worldDelta, const Vector3& boneRotEuler)
	{
		float yaw = DegreeToRadian(boneRotEuler.z);
		float pitch = DegreeToRadian(boneRotEuler.y);
		float roll = DegreeToRadian(boneRotEuler.x);
		float cZ = cosf(yaw), sZ = sinf(yaw);
		float cX = cosf(roll), sX = sinf(roll);
		float cY = cosf(pitch), sY = sinf(pitch);
		Vector3 xAxis = Vector3(cZ * cY - sZ * sX * sY, sZ * cY + cZ * sX * sY, -cX * sY);
		Vector3 yAxis = Vector3(-sZ * cX, cZ * cX, sX);
		Vector3 zAxis = Vector3(cZ * sY + sZ * sX * cY, sZ * sY - cZ * sX * cY, cX * cY);
		return Vector3(Vector3::Dot(worldDelta, xAxis), Vector3::Dot(worldDelta, yAxis), Vector3::Dot(worldDelta, zAxis));
	}

	static void GetAttachmentOffset(SpoonerEntity& sel, const GTAentity& parentEntity, const Vector3& newWorldPos)
	{
		if (sel.attachmentArgs.boneIndex >= 0)
		{
			Vector3 worldDelta = newWorldPos - sel.handle.GetPosition();
			Vector3 boneRot = ENTITY::GET_ENTITY_BONE_ROTATION(parentEntity.GetHandle(), sel.attachmentArgs.boneIndex);
			sel.attachmentArgs.offset = sel.attachmentArgs.offset + WorldDeltaToBoneRelative(worldDelta, boneRot);
		}
		else
		{
			sel.attachmentArgs.offset = parentEntity.GetOffsetGivenWorldCoords(newWorldPos);
		}
	}

// ═══════════════════════════════════════════════════════════════════
//  Script Thread Ticks
// ═══════════════════════════════════════════════════════════════════

	static void DrainPending_ScriptThread(SharedState& s)
	{
		SpoonerEntity& sel = selectedEntity;
		if (sel.handle.Exists())
		{
			GTAentity parentEntity(ENTITY::GET_ENTITY_ATTACHED_TO(sel.handle.Handle()));

			// Normal entity (not attached)
			if (!sel.attachmentArgs.isAttached)
			{
				if (s.pending.positionDirty) sel.handle.SetPosition(SpoonerMode::SnapPos(s.pending.positionVal));
				if (s.pending.rotationDirty) sel.handle.SetRotation(SpoonerMode::SnapRot(s.pending.rotationVal));
			}
			// Attached entity - converting to local offsets
			else if (parentEntity.Exists())
			{
				if (s.pending.positionDirty) GetAttachmentOffset(sel, parentEntity, s.pending.positionVal);
				if (s.pending.rotationDirty)
				{
					float oldWorldM[16], newWorldM[16], oldLocalM[16];
					Vector3 curWorldRot = sel.handle.Rotation_get();
					BuildTransformMatrix(Vector3(), curWorldRot, Vector3(1.0f, 1.0f, 1.0f), oldWorldM);
					BuildTransformMatrix(Vector3(), s.pending.rotationVal, Vector3(1.0f, 1.0f, 1.0f), newWorldM);
					BuildTransformMatrix(Vector3(), sel.attachmentArgs.rotation, Vector3(1.0f, 1.0f, 1.0f), oldLocalM);

					float worldT[16], temp[16], newLocalM[16];
					Mat4Transpose(oldWorldM, worldT);
					Mat4Mul(oldLocalM, worldT, temp);
					Mat4Mul(temp, newWorldM, newLocalM);

					Vector3 pos, newLocalRot, scale;
					DecomposeTransformMatrix(newLocalM, pos, newLocalRot, scale);
					sel.attachmentArgs.rotation = newLocalRot;
				}

				if (s.pending.positionDirty || s.pending.rotationDirty)
				{
					sel.handle.AttachTo(parentEntity, sel.attachmentArgs.boneIndex, sel.handle.GetIsCollisionEnabled(), sel.attachmentArgs.offset, sel.attachmentArgs.rotation);
				}
			}

			if (s.pending.scaleDirty) {
				sel.handle.SetScale(s.pending.scaleVal);
				// syncing scale so that it doesn't reset every time we grab the gizmo
				Entity entHandle = sel.handle.GetHandle();
				Submenus::EntityScaleState& state = [&]() -> Submenus::EntityScaleState& {
					switch (static_cast<EntityType>(sel.handle.Type()))
					{
					case EntityType::VEHICLE: return Submenus::_vehScale;
					case EntityType::PED:    return Submenus::_pedScale;
					default:                 return Submenus::_objScale;
					}
				}();
				state.handle = entHandle;
				state.scale = s.pending.scaleVal;
			}
		}
		s.pending = PendingWrites{};
	}

	// ── Snapshot ──────────────────────────────────────────────────

	static void RefreshSnapshot_ScriptThread(SharedState& s)
	{
		int renderingCam = CAM::GET_RENDERING_CAM();
		if (renderingCam != 0 && CAM::DOES_CAM_EXIST(renderingCam))
		{
			s.render.camCoord = CAM::GET_CAM_COORD(renderingCam);
			s.render.camRot   = CAM::GET_CAM_ROT(renderingCam, 2);
			s.render.camFov   = CAM::GET_CAM_FOV(renderingCam);
		}
		else
		{
			s.render.camCoord = CAM::GET_GAMEPLAY_CAM_COORD();
			s.render.camRot   = CAM::GET_GAMEPLAY_CAM_ROT(2);
			s.render.camFov   = CAM::GET_GAMEPLAY_CAM_FOV();
		}

		s.render.editingState = SpoonerMode::editingState;
		s.render.cursorModeEnabled = Settings::bCursorMode;
		s.render.gridSnapEnabled = Settings::bGridSnapEnabled;
		s.render.gridSnapSize = Settings::gridSnapSize;
		s.render.rotationSnapDegrees = Settings::rotationSnapDegrees;
		s.render.drawGrid = Settings::bDrawGrid;

		SpoonerEntity& sel = selectedEntity;
		s.cache.entityHandle = sel.handle.Handle();
		s.cache.entityValid = (s.cache.entityHandle != 0) && sel.handle.Exists();
		if (!s.cache.entityValid)
		{
			s.cache.position = Vector3{};
			s.cache.rotation = Vector3{};
			s.cache.scale = Vector3{1.0f, 1.0f, 1.0f};
			s.cache.entityFrozen = false;
			s.cache.entityCollision = true;
			s.cache.entityType = 0;
			s.cache.entityInDb = false;
			s.cache.entityHashName.clear();
			s.cache.entityAttached = false;
			s.cache.vehicleEngineOn = false;
			s.cache.vehicleLightsOn = false;
			return;
		}

		s.cache.position = sel.handle.GetPosition();
		s.cache.rotation = sel.handle.Rotation_get();
		s.cache.scale = sel.handle.GetScale();

		// Cache entity state for context menu
		s.cache.entityFrozen = sel.handle.IsPositionFrozen();
		s.cache.entityCollision = sel.handle.GetIsCollisionEnabled();
		s.cache.entityType = static_cast<int>(sel.handle.Type());
		s.cache.entityHashName = sel.hashName;
		s.cache.entityInDb = EntityManagement::GetEntityIndexInDb(sel) >= 0;
		s.cache.entityAttached = ENTITY::IS_ENTITY_ATTACHED(sel.handle.Handle());
		if (s.cache.entityType == 2)
		{
			s.cache.vehicleEngineOn = GET_IS_VEHICLE_ENGINE_RUNNING(sel.handle.Handle());
			BOOL lightsOn = FALSE, highbeamsOn = FALSE;
			GET_VEHICLE_LIGHTS_STATE(sel.handle.Handle(), &lightsOn, &highbeamsOn);
			s.cache.vehicleLightsOn = lightsOn != FALSE;
		}
		else
		{
			s.cache.vehicleEngineOn = false;
			s.cache.vehicleLightsOn = false;
		}
	}

	// ── Favourite Cache Refresh ────────────────────────────────────
	// Refresh the XML-backed lists immediately, then every 30 seconds.

	static void RefreshFavouriteCache_ScriptThread(FavouriteCache& cache)
	{
		cache.props.clear();
		{
			pugi::xml_document doc;
			if (doc.load_file((GetPathffA(Pathff::Main, true) + "FavouriteProps.xml").c_str()))
			{
				for (auto node = doc.child("FavouriteProps").first_child(); node; node = node.next_sibling())
					cache.props.push_back({ node.attribute("modelName").as_string(), GET_HASH_KEY(node.attribute("modelName").as_string()) });
			}
		}

		cache.peds.clear();
		{
			pugi::xml_document doc;
			if (doc.load_file((GetPathffA(Pathff::Main, true) + "FavouritePeds.xml").c_str()))
			{
				for (auto node = doc.child("FavouritePeds").first_child(); node; node = node.next_sibling())
					cache.peds.push_back({ node.attribute("customName").as_string(), node.attribute("hash").as_uint() });
			}
		}

		cache.vehicles.clear();
		{
			pugi::xml_document doc;
			if (doc.load_file((GetPathffA(Pathff::Main, true) + "AddedVehicleModels.xml").c_str()))
			{
				for (auto node = doc.child("AddedVehicleModels").first_child(); node; node = node.next_sibling())
					cache.vehicles.push_back({ node.attribute("customName").as_string(), node.attribute("modelHash").as_uint() });
			}
		}
	}

	static void RefreshDbCache_ScriptThread(SharedState& s)
	{
		s.dbEntityCache.clear();
		s.dbEntityCache.reserve(Databases::EntityDb.size());
		for (int i = 0; i < static_cast<int>(Databases::EntityDb.size()); i++)
		{
			auto& ent = Databases::EntityDb[i];
			if (ent.handle.Exists())
				s.dbEntityCache.push_back({ ent.hashName, ent.handle.Handle() });
		}
	}

	static std::optional<FavouriteCache> RefreshCaches_ScriptThread()
	{
		static DWORD lastRefresh = 0;
		static bool hasRefreshed = false;
		const DWORD now = GetTickCount();
		if (hasRefreshed && now - lastRefresh < 30000)
			return std::nullopt;
		lastRefresh = now;
		hasRefreshed = true;
		FavouriteCache cache;
		RefreshFavouriteCache_ScriptThread(cache);
		return cache;
	}

	static std::vector<QueuedCommand> DrainQueue_ScriptThread(SharedState& s)
	{
		std::vector<QueuedCommand> localQueue;
		localQueue.swap(s.cmds.queue);
		return localQueue;
	}

// ═══════════════════════════════════════════════════════════════════
//  Cursor Command Processing (dispatch table)
// ═══════════════════════════════════════════════════════════════════

	using CmdHandler = void(*)(const QueuedCommand&);

	static void Cmd_None(const QueuedCommand&) {}

	// ── RMB entity commands ──
	static void Cmd_RmbMenu_ManualEditing(const QueuedCommand&) { SpoonerMode::OpenMenu(SUB::SPOONER_MANUALEDITING); }
	static void Cmd_RmbMenu_Attachment(const QueuedCommand&)    { SpoonerMode::OpenMenu(SUB::SPOONER_ATTACHMENTOPS); }
	static void Cmd_RmbMenu_TaskSequence(const QueuedCommand&)  { SpoonerMode::OpenMenu(SUB::SPOONER_TASKSEQUENCE_TASKLIST); }
	static void Cmd_RmbMenu_Wardrobe(const QueuedCommand&)      { Submenus::SetSelectedEntityAsActivePed(); SpoonerMode::OpenMenu(SUB::COMPONENTS); }
	static void Cmd_RmbMenu_Animations(const QueuedCommand&)    { Submenus::SetSelectedEntityAsActivePed(); SpoonerMode::OpenMenu(SUB::ANIMATIONSUB); }
	static void Cmd_RmbMenu_Frozen(const QueuedCommand&)        { if (selectedEntity.handle.Exists()) selectedEntity.handle.FreezePosition(!selectedEntity.handle.IsPositionFrozen()); }
	static void Cmd_RmbMenu_Collision(const QueuedCommand&)     { if (selectedEntity.handle.Exists()) selectedEntity.handle.SetIsCollisionEnabled(!selectedEntity.handle.GetIsCollisionEnabled()); }
	static void Cmd_RmbMenu_Copy(const QueuedCommand&)
	{
		if (!selectedEntity.handle.Exists()) return;
		selectedEntity = EntityManagement::CopyEntity(
			selectedEntity,
			EntityManagement::GetEntityIndexInDb(selectedEntity) >= 0,
			true,
			Submenus::_copyEntTexterValue);
	}
	static void Cmd_RmbMenu_Delete(const QueuedCommand&)
	{
		if (!selectedEntity.handle.Exists()) return;
		selectedEntity.handle.RequestControl(600);
		EntityManagement::DeleteEntity(selectedEntity);
		SpoonerMode::ResetSelectedEntity();
		SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
	}
	static void Cmd_RmbMenu_PlaceOnGround(const QueuedCommand&) { if (selectedEntity.handle.Exists()) selectedEntity.handle.PlaceOnGround(); }
	static void Cmd_RmbMenu_DbToggle(const QueuedCommand&)
	{
		if (!selectedEntity.handle.Exists()) return;
		const int index = EntityManagement::GetEntityIndexInDb(selectedEntity);
		if (index >= 0)
			EntityManagement::RemoveEntityFromDb(selectedEntity);
		else
			EntityManagement::AddEntityToDb(selectedEntity, Settings::bAddToDbAsMissionEntities);
	}
	static void Cmd_RmbMenu_Detach(const QueuedCommand&)
	{
		if (selectedEntity.handle.Exists())
			EntityManagement::DetachEntity(selectedEntity);
	}
	static void Cmd_RmbMenu_Engine(const QueuedCommand&)
	{
		if (!selectedEntity.handle.Exists() || static_cast<EntityType>(selectedEntity.handle.Type()) != EntityType::VEHICLE) return;
		const BOOL running = GET_IS_VEHICLE_ENGINE_RUNNING(selectedEntity.handle.Handle());
		SET_VEHICLE_ENGINE_ON(selectedEntity.handle.Handle(), !running, true, true);
	}
	static void Cmd_RmbMenu_Lights(const QueuedCommand&)
	{
		if (!selectedEntity.handle.Exists() || static_cast<EntityType>(selectedEntity.handle.Type()) != EntityType::VEHICLE) return;
		BOOL lightsOn = FALSE, highbeamsOn = FALSE;
		GET_VEHICLE_LIGHTS_STATE(selectedEntity.handle.Handle(), &lightsOn, &highbeamsOn);
		SET_VEHICLE_LIGHTS(selectedEntity.handle.Handle(), lightsOn ? 4 : 3);
	}
	static void Cmd_RmbMenu_Repair(const QueuedCommand&)
	{
		if (selectedEntity.handle.Exists() && static_cast<EntityType>(selectedEntity.handle.Type()) == EntityType::VEHICLE)
			SET_VEHICLE_FIXED(selectedEntity.handle.Handle());
	}
	static void Cmd_RmbMenu_MenyooCustoms(const QueuedCommand&) { Submenus::SetSelectedEntityAsVehicleTarget(); SpoonerMode::OpenMenu(SUB::MODSHOP); }

	// ── World commands ──
	static void Cmd_World_TimePreset(const QueuedCommand& command)
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
	static void Cmd_World_WeatherSet(const QueuedCommand& command)
	{
		const int index = command.intPayload;
		if (index >= 0 && index < static_cast<int>(World::sWeatherNames.size()))
			World::SetWeather(World::sWeatherNames[index].second);
	}
	static void Cmd_World_WeatherReset(const QueuedCommand&) { World::ClearWeatherOverride(); }
	static void Cmd_World_SpeedSet(const QueuedCommand& command)   { SET_TIME_SCALE(command.floatPayload); }

	// ── Spawn commands ──
	static void Cmd_SpawnFavourite(const QueuedCommand& command)
	{
		if (g_PendingSpawn.active) return;

		REQUEST_MODEL(command.spawnPayload.modelHash);
		g_PendingSpawn = {
			command.spawnPayload.modelHash,
			command.spawnPayload.category,
			command.spawnPayload.name,
			GetTickCount(),
			true
		};
	}

	static void CheckPendingSpawns_ScriptThread()
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

	// ── Menu and view commands ──
	static void Cmd_OpenMenu(const QueuedCommand& command)
	{
		SpoonerMode::OpenMenu(command.intPayload, command.dbPayload);
	}
	static void Cmd_View_GridSnap(const QueuedCommand& command)
	{
		Settings::bGridSnapEnabled = command.floatPayload > 0.0f;
		if (Settings::bGridSnapEnabled)
			Settings::gridSnapSize = command.floatPayload;
	}
	static void Cmd_View_RotationSnap(const QueuedCommand& command) { Settings::rotationSnapDegrees = command.floatPayload; }
	static void Cmd_View_DrawGrid(const QueuedCommand&)       { Settings::bDrawGrid = !Settings::bDrawGrid; }
	static void Cmd_View_CursorMode(const QueuedCommand& command)
	{
		Settings::bCursorMode = command.intPayload != 0;
		if (!Settings::bCursorMode)
			SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
	}
	static void Cmd_CloseSpooner(const QueuedCommand&) { SpoonerMode::TurnOff(); }

	// ── Select / click commands ──
	static void Cmd_SelectEntity(const QueuedCommand& command)
	{
		if (!SpoonerCamera::camera.Exists()) return;

		GTAentity clicked = SpoonerCamera::camera.RaycastForEntity(
			Vector2(command.cursorScreenX, command.cursorScreenY), 0, 160.0f);
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
	static void Cmd_SelectEntityAndShowMenu(const QueuedCommand& command)
	{
		if (!SpoonerCamera::camera.Exists()) return;

		GTAentity clicked = SpoonerCamera::camera.RaycastForEntity(
			Vector2(command.cursorScreenX, command.cursorScreenY), 0, 160.0f);
		if (clicked.Exists())
		{
			SpoonerMode::SetAsSelectedEntity(clicked);
			SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
			g_ContextMenuReady = true;
			return;
		}

		SpoonerMode::ResetSelectedEntity();
		SpoonerMode::editingState.mode = SpoonerMode::eEditMode::Disabled;
		g_EmptySpaceMenuReady = true;
	}
	static void Cmd_EmptyMenu_PlaceEntityHere(const QueuedCommand& command)
	{
		if (!SpoonerCamera::camera.Exists()) return;

		const int targetHandle = command.dbPayload;
		auto entity = std::find_if(Databases::EntityDb.begin(), Databases::EntityDb.end(),
			[targetHandle](const SpoonerEntity& entry) { return entry.handle.GetHandle() == targetHandle; });
		if (entity == Databases::EntityDb.end() || !entity->handle.Exists()) return;

		if (entity->attachmentArgs.isAttached)
			EntityManagement::DetachEntity(*entity);

		const Vector3 cursorPosition = SpoonerCamera::camera.RaycastForCoord(
			Vector2(command.cursorScreenX, command.cursorScreenY), 0, 300.0f, 300.0f);
		entity->handle.RequestControlOnce();
		entity->handle.SetPosition(cursorPosition);
		entity->handle.PlaceOnGround();
	}

	static const CmdHandler s_cmdHandlers[] = {
		Cmd_None,
		Cmd_SelectEntity,
		Cmd_SelectEntityAndShowMenu,
		Cmd_RmbMenu_ManualEditing,
		Cmd_RmbMenu_Attachment,
		Cmd_RmbMenu_TaskSequence,
		Cmd_RmbMenu_Wardrobe,
		Cmd_RmbMenu_Animations,
		Cmd_RmbMenu_Frozen,
		Cmd_RmbMenu_Collision,
		Cmd_RmbMenu_Copy,
		Cmd_RmbMenu_Delete,
		Cmd_RmbMenu_PlaceOnGround,
		Cmd_RmbMenu_DbToggle,
		Cmd_RmbMenu_Detach,
		Cmd_RmbMenu_Engine,
		Cmd_RmbMenu_Lights,
		Cmd_RmbMenu_Repair,
		Cmd_RmbMenu_MenyooCustoms,
		Cmd_EmptyMenu_PlaceEntityHere,
		Cmd_World_TimePreset,
		Cmd_World_WeatherSet,
		Cmd_World_WeatherReset,
		Cmd_World_SpeedSet,
		Cmd_SpawnFavourite,
		Cmd_OpenMenu,
		Cmd_View_GridSnap,
		Cmd_View_RotationSnap,
		Cmd_View_DrawGrid,
		Cmd_View_CursorMode,
		Cmd_CloseSpooner,
	};
	static const int s_cmdHandlerCount = sizeof(s_cmdHandlers) / sizeof(s_cmdHandlers[0]);
	static_assert(s_cmdHandlerCount == static_cast<int>(CursorCommand::CloseSpooner) + 1, "Cursor command table is out of sync");

	static void ProcessCursorCommand(const QueuedCommand& command)
	{
		if (command.cmd == CursorCommand::None) return;

		const int index = static_cast<int>(command.cmd);
		if (index >= 0 && index < s_cmdHandlerCount)
			s_cmdHandlers[index](command);
	}

// ═══════════════════════════════════════════════════════════════════
//  Main Tick
// ═══════════════════════════════════════════════════════════════════

	void Tick()
	{
		if (!g_Visible) return;

		bool capturedGizmoOver = false, capturedGizmoUsing = false, capturedContextSearchFocused = false;
		bool capturedCursorMode = false;
		SpoonerMode::eEditMode capturedEditMode = SpoonerMode::eEditMode::Disabled;

		auto refreshedFavourites = RefreshCaches_ScriptThread();
		std::vector<QueuedCommand> commands;

		{
			std::lock_guard<std::mutex> lock(g_Mutex);

			// Write any pending gizmo changes to selected entity
			DrainPending_ScriptThread(g_Shared);
			commands = DrainQueue_ScriptThread(g_Shared);
		}

		// Execute game-native commands without holding the render-state mutex.
		// Some commands yield with WAIT(), so keeping the mutex held here would
		// block the D3D render callback while the script thread is suspended.
		for (const auto& command : commands)
			ProcessCursorCommand(command);
		CheckPendingSpawns_ScriptThread();

		{
			std::lock_guard<std::mutex> lock(g_Mutex);

			// Update current state cache
			RefreshSnapshot_ScriptThread(g_Shared);
			RefreshDbCache_ScriptThread(g_Shared);

			if (refreshedFavourites)
				g_Shared.favouriteCache = std::move(*refreshedFavourites);

			capturedGizmoOver = g_Shared.render.gizmoOver;
			capturedGizmoUsing = g_Shared.render.gizmoUsing;
			capturedContextSearchFocused = g_Shared.render.ctxSearchFocused;
			capturedCursorMode = g_Shared.render.cursorModeEnabled;
			capturedEditMode = g_Shared.render.editingState.mode;
		}

		// Disable player controls when using the gizmo or in cursor mode
		if (g_Visible && (capturedCursorMode || capturedContextSearchFocused || capturedEditMode == SpoonerMode::eEditMode::Gizmo || capturedGizmoOver || capturedGizmoUsing))
			PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
	}

// ═══════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════

	bool Initialize()
	{
		if (D3D11Hook::IsInitialized())
			return true;

		g_ShuttingDown = false;
		return D3D11Hook::Initialize(OnRender);
	}

	void Shutdown()
	{
		g_ShuttingDown = true;
		g_Visible = false;

		if (g_ImGuiInitialized)
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			g_ImGuiInitialized = false;
		}

		D3D11Hook::Shutdown();
	}

	void SetVisible(bool visible)
	{
		g_Visible = visible;
		D3D11Hook::SetMenuVisible(visible);
		if (!visible && g_ImGuiInitialized)
		{
			ImGui::GetIO().MouseDrawCursor = false;
			while (ShowCursor(false) >= 0);
		}
	}

	bool IsVisible()
	{
		return g_Visible;
	}
}
