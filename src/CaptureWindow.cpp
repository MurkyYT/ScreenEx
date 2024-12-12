#include <time.h>
#include <shlobj.h>
#include <dwmapi.h>
#include "WindowsUtils.h"
#include "CaptureWindow.h"
#include "Logger.h"
#include "Drawing.h"
#include "Intersection.h"
#include "resource.h"

#pragma comment(lib ,"Dwmapi.lib")

#define CLASS_NAME L"CaptureWindow_Class"
#define WINDOW_TITLE L"ScreenEx Overlay"
#define START_HIDING_TIMER_ID 0

std::wstring CaptureWindow::s_screenshotsFolder;
IAppVisibility* CaptureWindow::s_pAppVis = NULL;

CaptureWindow::CaptureWindow(Bitmap* bitmap, BOOL* windowOpen, Config* config) :
	m_bitmap(bitmap), m_pConfig(config), m_hWnd(NULL), m_pbWindowOpen(windowOpen), m_dragEnd({ 0,0 }), m_selectionRect({ 0,0,0,0 }),
	m_darkenedBitmap(NULL), m_dockRect({0,0,0,}), m_closeRect({ 0,0,0, }), m_rectangleSelectionRect({ 0,0,0, }),m_hOriginalBitmap(NULL),
	m_windowSelectionRect({ 0,0,0, }), m_mouseInsideDock(FALSE), m_prevFocusedWindow(NULL)
{
	if (s_screenshotsFolder.empty())
	{
		WCHAR picutresPath[MAX_PATH];
		SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, picutresPath);

		s_screenshotsFolder = std::wstring(picutresPath) + L"\\ScreenEx";

		if (!DirectoryExists(s_screenshotsFolder.c_str()))
			CreateDirectory(s_screenshotsFolder.c_str(), NULL);
	}
	if (!s_pAppVis)
	{
		if (CoCreateInstance(CLSID_AppVisibility, NULL, CLSCTX_INPROC_SERVER,
			IID_IAppVisibility, (void**)&s_pAppVis) != S_OK)
		{
			return;
		}
	}
	m_darkenedBitmap = m_bitmap->Copy();
	m_darkenedBitmap->DarkenImage(60, NULL);
	m_hOriginalBitmap = m_bitmap->GetHBITMAP();

	m_selectionRect = { -1,-1,-1,-1 };

	for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT))
		GetWindowBounds(hwnd, m_activeWindowsRect);

	m_prevFocusedWindow = GetForegroundWindow();

	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = CaptureWindow::WndProc;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SNIPPLUSPLUS));
	wcex.lpszClassName = CLASS_NAME;
	
	if (!RegisterClassEx(&wcex)) {
		if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			return;
	}

	m_hWnd = CreateWindowEx
	(
		WS_EX_TOPMOST,
		CLASS_NAME,
		WINDOW_TITLE,
		WS_POPUP,
		GetSystemMetrics(SM_XVIRTUALSCREEN),
		GetSystemMetrics(SM_YVIRTUALSCREEN),
		m_bitmap->GetWidth(),
		m_bitmap->GetHeight(),
		NULL,
		NULL,
		GetModuleHandle(NULL),
		this
	);

	UpdateDock();

	ShowWindow(m_hWnd, SW_SHOW);
	SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	BOOL startVisible = FALSE;
	s_pAppVis->IsLauncherVisible(&startVisible);
	SetTimer(m_hWnd, START_HIDING_TIMER_ID, 1, NULL);

	LOGD((L"[SnipWindow.cpp] Created new snip window (" + std::to_wstring((UINT_PTR)this) + L")").c_str());
}

void CaptureWindow::UpdateDock()
{
	POINT dockBar{ GetSystemMetrics(SM_CXFULLSCREEN) / 2 - 200 / 2,10 };
	ScreenToClient(m_hWnd, &dockBar);

	m_dockRect = { dockBar.x,dockBar.y,dockBar.x + 200, dockBar.y + 50 };

	m_closeRect.top = m_dockRect.top + 10;
	m_closeRect.left = m_dockRect.right - 40;
	m_closeRect.right = m_dockRect.right - 10;
	m_closeRect.bottom = m_dockRect.top + 40;

	m_rectangleSelectionRect.top = m_dockRect.top + 10;
	m_rectangleSelectionRect.left = m_dockRect.left + 10;
	m_rectangleSelectionRect.right = m_dockRect.left + 40;
	m_rectangleSelectionRect.bottom = m_dockRect.top + 40;

	m_windowSelectionRect.top = m_rectangleSelectionRect.top;
	m_windowSelectionRect.left = m_rectangleSelectionRect.right + 10;
	m_windowSelectionRect.right = m_rectangleSelectionRect.right + 40;
	m_windowSelectionRect.bottom = m_rectangleSelectionRect.bottom;
}

BOOL CaptureWindow::IsActualWindow(HWND hWnd)
{
	int length = GetWindowTextLength(hWnd);
	std::wstring windowTitle;
	windowTitle.resize(length + 1);
	GetWindowText(hWnd, (wchar_t*)windowTitle.data(), length + 1);

	if (!IsWindowVisible(hWnd))
		return FALSE;

	if (!IsWindow(hWnd))
		return FALSE;

	LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

	if (exStyle & WS_EX_TOOLWINDOW)
		return FALSE;

	HWND owner = GetWindow(hWnd, GW_OWNER);
	HWND lastOwner = owner;
	while (owner)
	{
		lastOwner = owner;
		owner = GetWindow(owner, GW_OWNER);
	}

	if (lastOwner == GetDesktopWindow())
		return FALSE;

	if (lastOwner && !IsWindowVisible(lastOwner))
		return FALSE;

	LONG style = GetWindowLong(hWnd, GWL_STYLE);

	WINDOWPLACEMENT wndPlc{};
	GetWindowPlacement(hWnd, &wndPlc);

	if (wndPlc.showCmd == SW_HIDE)
		return FALSE;

	DWORD dwPID;
	std::wstring path;
	DWORD lpdwSize = MAX_PATH + 1;
	path.resize(lpdwSize);
	GetWindowThreadProcessId(hWnd, &dwPID);

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);

	BOOL bFile = QueryFullProcessImageName(hProcess, 0, (wchar_t*)path.data(), &lpdwSize);

	CloseHandle(hProcess);

	size_t startIndex = path.find_last_of(L"\\") + 1;
	std::wstring name = path.substr(startIndex);

	if (name == L"explorer.exe")
		if (windowTitle == L"")
			return FALSE;

	if ((style & WS_CHILD))
		return FALSE;

	BOOL isCloaked = FALSE;
	DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(BOOL));

	if (isCloaked)
		return FALSE;

	return TRUE;
}

void CaptureWindow::GetWindowBounds(HWND hwnd, std::vector<RECT>& rects)
{
	if (!IsActualWindow(hwnd))
		return;

	DWORD winid = 0;
	GetWindowThreadProcessId(hwnd, &winid);
	if (winid == NULL)
		return;

	RECT rect{};
	if (DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(RECT)) != S_OK)
	{
		if (!GetWindowRect(hwnd, &rect))
			return;
	}

	rects.push_back(rect);
}

void CaptureWindow::Close()
{
	if (!this)
		return;

	DestroyWindow(m_hWnd);
	(*m_pbWindowOpen) = FALSE;
	LOGD((L"[SnipWindow.cpp] Closed snip window (" + std::to_wstring((UINT_PTR)this) + L")").c_str());
}

LRESULT CALLBACK CaptureWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CaptureWindow* window = nullptr;

	if (msg == WM_NCCREATE)
	{
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		window = (CaptureWindow*)pCreate->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
		window->m_hWnd = hwnd;
	}
	else
		window = (CaptureWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (window)
	{
		switch (msg)
		{
		case WM_TIMER:
		{
			BOOL startVisible = FALSE;
			s_pAppVis->IsLauncherVisible(&startVisible);
			if (!startVisible)
			{
				KillTimer(hwnd, START_HIDING_TIMER_ID);
				ShowWindow(FindWindow(L"Shell_TrayWnd", NULL), SW_HIDE);
				break;
			}
			INPUT ip[2]{};
			ip[0].type = INPUT_KEYBOARD;
			ip[0].ki.wVk = VK_RWIN;

			ip[1].type = INPUT_KEYBOARD;
			ip[1].ki.wVk = VK_RWIN;
			ip[1].ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(2, &ip[0], sizeof(INPUT));
		}
		break;
		case WM_LBUTTONDOWN:
		{
			INT x = LOWORD(lParam), y = HIWORD(lParam);
			window->HandleLeftClick(x, y);
		}
		break;
		case WM_SETCURSOR:
			if ((window->m_selectionRect.X != -1 &&
				window->m_selectionRect.Y != -1 &&
				window->m_selectionRect.Width != -1 &&
				window->m_selectionRect.Height != -1) ||
				window->m_mouseInsideDock)
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			else
				SetCursor(LoadCursor(NULL, IDC_CROSS));
			break;
		case WM_MOUSEMOVE:
		{
			window->HandleMouseMove(lParam, hwnd);
		}
		break;
		case WM_LBUTTONUP:
		{
			if (window->m_mouseInsideDock)
				break;

			window->m_dragStart.x = -1;
			window->m_dragStart.y = -1;
			window->m_bitmap->Crop(window->m_selectionRect);

			if ((*window->m_pConfig).saveScreenshots) {
				time_t rawtime;
				struct tm timeinfo;
				time(&rawtime);
				localtime_s(&timeinfo, &rawtime);
				wchar_t timeBuffer[1024];
				wcsftime(timeBuffer, 1024, L"\\%Y%m%d_%H%M%S.png", &timeinfo);

				std::wstring savePath = s_screenshotsFolder + timeBuffer;

				LOGI(L"[SnipWindow.cpp] Saving screenshot to: '" + savePath + L"'");

				window->m_bitmap->Save(savePath.c_str(), L"image/png");
			}

			if ((*window->m_pConfig).copyToClipboard) {
				HBITMAP newBMP = window->m_bitmap->GetHBITMAP();

				BITMAP bm{};
				GetObject(newBMP, sizeof(bm), &bm);

				HBITMAP hBitmap_copy = CreateBitmap(bm.bmWidth, bm.bmHeight, bm.bmPlanes, bm.bmBitsPixel, NULL);

				HDC srcDC = CreateCompatibleDC(NULL);
				HDC newDC = CreateCompatibleDC(NULL);

				HBITMAP srcBitmap = (HBITMAP)SelectObject(srcDC, newBMP);
				HBITMAP newBitmap = (HBITMAP)SelectObject(newDC, hBitmap_copy);

				BitBlt(newDC, 0, 0, bm.bmWidth, bm.bmHeight, srcDC, 0, 0, SRCCOPY);

				OpenClipboard(hwnd);
				EmptyClipboard();
				SetClipboardData(CF_BITMAP, hBitmap_copy);
				CloseClipboard();

				DeleteDC(srcDC);
				DeleteDC(newDC);
				DeleteObject(newBMP);
				DeleteObject(srcBitmap);
				DeleteObject(newBitmap);
			}

			window->Close();
		}
		break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			HDC bufferedDC = CreateCompatibleDC(NULL);
			INT width, height;

			window->DrawImage(bufferedDC, &width, &height);

			if (window->m_dragStart.x == -1 && window->m_dragStart.y == -1)
				window->DrawDock(bufferedDC);

			BitBlt(hdc, 0, 0, width, height, bufferedDC, 0, 0, SRCCOPY);

			DeleteDC(bufferedDC);

			EndPaint(hwnd, &ps);
		}
		break;
		case WM_KILLFOCUS:
		{
			SetForegroundWindow(hwnd);
			BringWindowToTop(hwnd);
			SetActiveWindow(hwnd);
			SetFocus(hwnd);
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		break;
		case WM_ACTIVATE:
		{
			if (wParam)
			{
				if (!window->m_focusAttemptsLeft)
				{
					window->Close();
					break;
				}
				window->m_focusAttemptsLeft--;
			}
		}
		break;
		case WM_DESTROY:
		{
			delete window->m_bitmap;
			delete window->m_darkenedBitmap;
			DeleteObject(window->m_hOriginalBitmap);
			ShowWindow(FindWindow(L"Shell_TrayWnd", NULL), SW_SHOW);
			SetForegroundWindow(window->m_prevFocusedWindow);
		}
		break;
		default:
			break;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CaptureWindow::HandleLeftClick(INT x, INT y)
{
	POINT curPos = { x,y };
	if (!m_mouseInsideDock) {
		if (m_captureType == RECTANGLE_SELECTION)
			m_dragStart = curPos;
	}
	else
	{
		if (Intersection::IsPointInsde(m_closeRect,curPos))
			Close();

		if (Intersection::IsPointInsde(m_windowSelectionRect, curPos))
			m_captureType = WINDOW_SELECTION;

		if (Intersection::IsPointInsde(m_rectangleSelectionRect, curPos))
		{
			m_captureType = RECTANGLE_SELECTION;
			m_dragStart = { -1, -1 };
			m_selectionRect = { -1,-1,-1,-1 };
		}

		InvalidateRect(m_hWnd, NULL, FALSE);
	}
}

void CaptureWindow::DrawDock(HDC bufferedDC)
{
	POINT curPos;
	GetCursorPos(&curPos);
	ScreenToClient(m_hWnd, &curPos);

	Gdiplus::Graphics g(bufferedDC);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	Gdiplus::Rect dockRect(m_dockRect.left, m_dockRect.top, 200, 50);
	Gdiplus::Color dockBGColor(255, 31, 31, 31);
	Gdiplus::Color outlineColor(64, 64, 64);
	Drawing::FillRoundRect(&g, dockRect, dockBGColor, 10);

	Gdiplus::Color whiteColor(255, 255, 255, 255);
	Gdiplus::Pen whitePen(whiteColor);

	Gdiplus::Point leftTopX{ m_closeRect.left + 5,m_closeRect.top + 5 };
	Gdiplus::Point rightBottomX{ m_closeRect.right - 5,m_closeRect.bottom - 5 };
	Gdiplus::Point rightTopX{ m_closeRect.right - 5,m_closeRect.top + 5 };
	Gdiplus::Point leftBottomX{ m_closeRect.left + 5,m_closeRect.bottom - 5 };

	Gdiplus::Rect closeRect(m_closeRect.left, m_closeRect.top,
		m_closeRect.right - m_closeRect.left, 
		m_closeRect.bottom - m_closeRect.top);

	if (Intersection::IsPointInsde(closeRect, curPos))
	{
		Drawing::FillRoundRect(&g, closeRect, outlineColor, 5);
	}

	g.DrawLine(&whitePen, leftTopX, rightBottomX);
	g.DrawLine(&whitePen, leftBottomX, rightTopX);

	Gdiplus::Rect rectangleSelectionRect(m_rectangleSelectionRect.left + 5,
		m_rectangleSelectionRect.top + 5,
		m_rectangleSelectionRect.right - m_rectangleSelectionRect.left - 10,
		m_rectangleSelectionRect.bottom - m_rectangleSelectionRect.top - 10);

	Gdiplus::Rect rectangleHoverSelectionRect(m_rectangleSelectionRect.left,
		m_rectangleSelectionRect.top,
		m_rectangleSelectionRect.right - m_rectangleSelectionRect.left,
		m_rectangleSelectionRect.bottom - m_rectangleSelectionRect.top);

	if (Intersection::IsPointInsde(rectangleHoverSelectionRect, curPos)
		|| m_captureType == RECTANGLE_SELECTION)
	{
		Drawing::FillRoundRect(&g, rectangleHoverSelectionRect, outlineColor, 5);
	}
	Drawing::DrawRoundRect(&g, rectangleSelectionRect, whiteColor, 5, 1);

	Gdiplus::Rect windowSelectionRect(m_windowSelectionRect.left + 5,
		m_windowSelectionRect.top + 5,
		m_windowSelectionRect.right - m_windowSelectionRect.left - 10,
		m_windowSelectionRect.bottom - m_windowSelectionRect.top - 10);

	Gdiplus::Rect windowHoverSelectionRect(m_windowSelectionRect.left,
		m_windowSelectionRect.top,
		m_windowSelectionRect.right - m_windowSelectionRect.left,
		m_windowSelectionRect.bottom - m_windowSelectionRect.top);

	if (Intersection::IsPointInsde(windowHoverSelectionRect,curPos)
		|| m_captureType == WINDOW_SELECTION)
	{
		Drawing::FillRoundRect(&g, windowHoverSelectionRect, outlineColor, 5);
	}
	Drawing::DrawRoundRect(&g, windowSelectionRect, whiteColor, 5, 1);

	Gdiplus::Point windowTitleBarStart(windowSelectionRect.X, windowSelectionRect.Y + 5);
	Gdiplus::Point windowTitleBarEnd(windowSelectionRect.X + windowSelectionRect.Width - 1, windowSelectionRect.Y + 5);

	g.DrawLine(&whitePen, windowTitleBarStart, windowTitleBarEnd);

	Gdiplus::Color selectionColor(255, 76, 194, 255);
	Gdiplus::Pen selectionPen(selectionColor);

	Gdiplus::Point selectedLineStart{};
	Gdiplus::Point selectedLineEnd{};

	switch (m_captureType)
	{
	case RECTANGLE_SELECTION:
		selectedLineStart.X = rectangleSelectionRect.X + 5;
		selectedLineStart.Y = rectangleSelectionRect.Y + rectangleSelectionRect.Height + 5;
		selectedLineEnd.X = rectangleSelectionRect.X + rectangleSelectionRect.Width - 5;
		selectedLineEnd.Y = rectangleSelectionRect.Y + rectangleSelectionRect.Height + 5;
		break;
	case WINDOW_SELECTION:
		selectedLineStart.X = windowSelectionRect.X + 5;
		selectedLineStart.Y = windowSelectionRect.Y + windowSelectionRect.Height + 5;
		selectedLineEnd.X = windowSelectionRect.X + windowSelectionRect.Width - 5;
		selectedLineEnd.Y = windowSelectionRect.Y + windowSelectionRect.Height + 5;
		break;
	default:
		break;
	}

	g.DrawLine(&selectionPen, selectedLineStart, selectedLineEnd);

	Drawing::DrawRoundRect(&g, dockRect, outlineColor, 10, 1);
}

void CaptureWindow::HandleMouseMove(LPARAM& lParam, HWND hwnd)
{
	INT x = LOWORD(lParam), y = HIWORD(lParam);
	POINT curPos = { x,y };

	m_mouseInsideDock =
		Intersection::IsPointInsde(m_dockRect,curPos) &&
		m_dragStart.x == -1 && m_dragStart.y == -1;

	if (m_mouseInsideDock)
	{
		InvalidateRect(hwnd, NULL, FALSE);
		return;
	}

	switch (m_captureType)
	{
	case RECTANGLE_SELECTION:
	{
		if (m_dragStart.x != -1 && m_dragStart.y != -1)
		{
			m_dragEnd = { x,y };

			LONG top, left, right, bottom;

			top = m_dragStart.y > m_dragEnd.y ? m_dragEnd.y : m_dragStart.y;
			left = m_dragStart.x > m_dragEnd.x ? m_dragEnd.x : m_dragStart.x;
			bottom = m_dragStart.y < m_dragEnd.y ? m_dragEnd.y : m_dragStart.y;
			right = m_dragStart.x < m_dragEnd.x ? m_dragEnd.x : m_dragStart.x;

			m_selectionRect.X = left;
			m_selectionRect.Y = top;
			m_selectionRect.Width = right - left;
			m_selectionRect.Height = bottom - top;

			InvalidateRect(hwnd, NULL, FALSE);
		}
	}
	break;
	case WINDOW_SELECTION:
	{
		POINT pnt{ x,y };
		ClientToScreen(hwnd, &pnt);
		BOOL found = FALSE;
		for (size_t i = 0; i < m_activeWindowsRect.size(); i++)
		{
			RECT current = m_activeWindowsRect[i];

			if (Intersection::IsPointInsde(current,pnt))
			{
				pnt = { current.left ,current.top };
				ScreenToClient(hwnd, &pnt);
				INT width = current.right - current.left, height = current.bottom - current.top;
				if (m_selectionRect.X != pnt.x || m_selectionRect.Y != pnt.y
					|| m_selectionRect.Width != width || m_selectionRect.Height != height)
				{
					m_selectionRect.X = pnt.x;
					m_selectionRect.Y = pnt.y;
					m_selectionRect.Width = width;
					m_selectionRect.Height = height;
					InvalidateRect(hwnd, NULL, FALSE);
				}
				found = TRUE;
				break;
			}
		}
		if (!found && m_selectionRect.X != -1 && m_selectionRect.Y != -1 && m_selectionRect.Width != -1 && m_selectionRect.Height != -1)
		{
			m_selectionRect = { -1,-1,-1,-1 };
			InvalidateRect(hwnd, NULL, FALSE);
		}
	}
	break;
	default:
		break;
	}
}

void CaptureWindow::DrawImage(HDC hdc, INT* width, INT* height)
{
	HBITMAP darkenedBmp = m_darkenedBitmap->GetHBITMAP();

	BITMAP bm{};
	GetObject(darkenedBmp, sizeof(bm), &bm);

	HDC overlayDC = CreateCompatibleDC(NULL);

	HGDIOBJ oldBuffered = SelectObject(hdc, darkenedBmp);
	HGDIOBJ oldOverlay = SelectObject(overlayDC, m_hOriginalBitmap);

	if (m_selectionRect.X != -1 && m_selectionRect.Y != -1 && m_selectionRect.Width != -1 && m_selectionRect.Height != -1)
	{
		BitBlt(hdc,
			m_selectionRect.X,
			m_selectionRect.Y,
			m_selectionRect.Width,
			m_selectionRect.Height,
			overlayDC,
			m_selectionRect.X,
			m_selectionRect.Y,
			SRCCOPY);

		Gdiplus::Graphics g(hdc);
		Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 255, 255));
		Gdiplus::Pen pen(&brush, 2);
		g.DrawRectangle(&pen, m_selectionRect);
	}

	(*width) = bm.bmWidth;
	(*height) = bm.bmHeight;

	DeleteDC(overlayDC);
	DeleteObject(darkenedBmp);
	DeleteObject(oldBuffered);
	DeleteObject(oldOverlay);
}
