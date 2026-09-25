#include "Engine_pch.h"

#include "Input.h"
#include "imgui.h"

//--- グローバル変数
BYTE g_keyTable[256];
BYTE g_oldTable[256];
HWND g_hWnd;
bool g_isMouseLock = false;
float g_mouseDeltaX = 0;
float g_mouseDeltaY = 0;

HRESULT InitInput(HWND hWnd)
{
	g_hWnd = hWnd;
	// キーボードの状態を取得
	GetKeyboardState(g_keyTable);
	return S_OK;
}
void UninitInput()
{
}
void UpdateInput()
{
	// 古いキーボードの状態を更新
	memcpy_s(g_oldTable, sizeof(g_oldTable), g_keyTable, sizeof(g_keyTable));
	// 現在のキーボードの状態を取得
	GetKeyboardState(g_keyTable);

	// マウスロックの切り替え
	if (IsKeyTrigger(VK_ESCAPE)) {
		SetMouseLock(false);
	}
	if (IsKeyTrigger(VK_LBUTTON)) {
		// ImGuiが初期化されていて、かつUI上にマウスカーソルがない時だけロックする
		if (ImGui::GetCurrentContext() != nullptr && !ImGui::GetIO().WantCaptureMouse) {
			SetMouseLock(true);
		}
	}
	// マウス移動量の初期化
	g_mouseDeltaX = 0;
	g_mouseDeltaY = 0;

	if (g_isMouseLock && g_hWnd)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(g_hWnd, &pt);

		RECT rc;
		GetClientRect(g_hWnd, &rc);
		int cx = (rc.right - rc.left) / 2;
		int cy = (rc.bottom - rc.top) / 2;

		g_mouseDeltaX = pt.x - cx;
		g_mouseDeltaY = pt.y - cy;

		// マウスカーソルを画面中央に戻す	
		POINT center = { cx, cy };
		ClientToScreen(g_hWnd, &center);
		SetCursorPos(center.x, center.y);
	}
}

bool IsKeyPress(BYTE key)
{
	return g_keyTable[key] & 0x80;
}
bool IsKeyTrigger(BYTE key)
{
	return (g_keyTable[key] ^ g_oldTable[key]) & g_keyTable[key] & 0x80;
}
bool IsKeyRelease(BYTE key)
{
	return (g_keyTable[key] ^ g_oldTable[key]) & g_oldTable[key] & 0x80;
}
bool IsKeyRepeat(BYTE key)
{
	return false;
}

void SetMouseLock(bool lock)
{
	if (g_isMouseLock == lock) return;
	g_isMouseLock = lock;

	if (g_isMouseLock)
	{
		// マウスカーソルを画面中央に設定
		if (g_hWnd) {
			RECT rc;
			GetClientRect(g_hWnd, &rc);
			POINT center = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
			ClientToScreen(g_hWnd, &center);
			SetCursorPos(center.x, center.y);
		}
		ShowCursor(FALSE);
	}
	else
	{
		ShowCursor(TRUE);
	}
}

long GetMouseDeltaX() { return g_mouseDeltaX; }
long GetMouseDeltaY() { return g_mouseDeltaY; }