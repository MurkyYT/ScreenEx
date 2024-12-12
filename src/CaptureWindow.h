#pragma once

#include <Windows.h>
#include <shobjidl_core.h>
#include <vector>
#include "Bitmap.h"
#include "Config.h"

enum CaptureType
{
	RECTANGLE_SELECTION,
	WINDOW_SELECTION
};

class CaptureWindow
{
public:
	CaptureWindow(Bitmap* bitmap, BOOL* windowOpen, Config* config);
	void UpdateDock();
	void Close();
private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static void GetWindowBounds(HWND hwnd, std::vector<RECT>& rects);
	static BOOL IsActualWindow(HWND hWnd);
	static std::wstring s_screenshotsFolder;
	static IAppVisibility* s_pAppVis;
	void HandleLeftClick(INT x, INT y);
	void HandleMouseMove(LPARAM& lParam, HWND hwnd);
	void DrawDock(HDC bufferedDC);
	void DrawImage(HDC hdc, INT* width, INT* height);
	std::vector<RECT> m_activeWindowsRect;
	BOOL m_mouseInsideDock;
	RECT m_dockRect;
	RECT m_closeRect;
	RECT m_rectangleSelectionRect;
	RECT m_windowSelectionRect;
	HWND m_hWnd;
	HWND m_prevFocusedWindow;
	CaptureType m_captureType = RECTANGLE_SELECTION;
	Config* m_pConfig;
	Bitmap* m_bitmap;
	HBITMAP m_hOriginalBitmap;
	Bitmap* m_darkenedBitmap;
	UINT m_focusAttemptsLeft = 2;
	POINT m_dragStart = { -1,-1 };
	POINT m_dragEnd;
	Gdiplus::Rect m_selectionRect;
	BOOL* m_pbWindowOpen;
};