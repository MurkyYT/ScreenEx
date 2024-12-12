#pragma region Includes
#include <windows.h>
#define GDIPVER 0x0110
#include <gdiplus.h>
#include "resource.h"
#include "Bitmap.h"
#include "Logger.h"
#include "CaptureWindow.h"
#include "Config.h"
#include "DarkMode.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma endregion

#pragma region Defines
#define VER_MAJOR 1
#define VER_MINOR 0
#define VER_PATCH 0

#define REG_SETTINGS_PATH L"Software\\Murky\\ScreenEx"
#define WM_TRAY (WM_USER+1)
#define TRAY_ICON_ID 0xEB6E16

#define VK_S 0x53
#pragma endregion

#pragma region GDIPlus
ULONG_PTR gdiToken;
const Gdiplus::GdiplusStartupInput gdiInput;
Gdiplus::GdiplusStartupOutput gdiOutput;
#pragma endregion

#pragma region Constants
CONST UINT WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
#pragma endregion

#pragma region Globals
HHOOK g_hKeyboardHook = NULL;
BOOL g_captureStarted = FALSE;
HKEY g_settingsKey = NULL;
NOTIFYICONDATA g_niData;
HWND g_trayHwnd;
Config g_Config;
std::wstring g_verString = std::to_wstring(VER_MAJOR) + L"."
+ std::to_wstring(VER_MINOR) + L"."
+ std::to_wstring(VER_PATCH);
CaptureWindow* g_window = NULL;
#pragma endregion

#pragma region Declarations
void StartCapture();
void LoadSettings();
void SaveSettings();
void ShowRightClickMenu(HWND hwnd);
void SetAutoStartup(BOOL enabled);
void RegisterTray();
void DeregisterTray();
static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK TrayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#pragma endregion

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd
)
{

#ifndef NDEBUG
	AllocConsole();
	FILE* fp = nullptr;
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
#endif

	HRESULT hRes = CoInitialize(NULL);

	if(hRes != S_OK)
	{
		LOGI(L"[main.cpp] Couldn't initialize COM (" + std::to_wstring(hRes) + L")");
		MessageBox(NULL, L"Error while initializing COM", L"ERROR", MB_ICONERROR | MB_OK);
		return -1;
	}

	if (RegCreateKey(HKEY_CURRENT_USER, REG_SETTINGS_PATH, &g_settingsKey) != ERROR_SUCCESS)
	{
		LOGI(L"[main.cpp] Couldn't open registry settings (" + std::to_wstring(GetLastError()) + L")");
		MessageBox(NULL, L"Error occured while reading the settings", L"ERROR", MB_ICONERROR | MB_OK);
		return -1;
	}
	else
		LOGI(L"[main.cpp] Opened registry settings successfully");

	LoadSettings();

	Logger::Enabled = g_Config.saveLogs;

	LOGI(L"[main.cpp] ScreenEx v" + g_verString + L" has started");

	Gdiplus::Status gdiStatus = Gdiplus::GdiplusStartup(&gdiToken, &gdiInput, &gdiOutput);
	if (gdiStatus == Gdiplus::Status::Ok)
		LOGI(L"[main.cpp] GDI Plus was initialized successfully");
	else
	{
		LOGI(L"[main.cpp] GDIPlus could not be initialized (" + std::to_wstring(gdiStatus) + L")");
		MessageBox(NULL, L"Error occured when initializing GDIPlus", L"ERROR", MB_ICONERROR | MB_OK);
		return -1;
	}

	g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

	if (g_hKeyboardHook == NULL)
	{
		LOGI(L"[main.cpp] Keyboard could not be hooked (" + std::to_wstring(GetLastError()) + L")");
		MessageBox(NULL, L"Error occured while hooking the keyboard", L"ERROR", MB_ICONERROR | MB_OK);
		return -1;
	}
	else
		LOGI(L"[main.cpp] Keyboard was hooked successfully");

	SetAutoStartup(g_Config.startWithWindows);

	RegisterTray();

	InitDarkMode();
	AllowDarkModeForApp(TRUE);

	MSG msg{};

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(g_hKeyboardHook);
	Gdiplus::GdiplusShutdown(gdiToken);
	SaveSettings();
	RegCloseKey(g_settingsKey);
	CoUninitialize();
	LOGI(L"[main.cpp] ScreenEx has been shut-down");

	return 0;
}

void RegisterTray()
{
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = TrayProc;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SNIPPLUSPLUS));
	wcex.lpszClassName = L"ScreenExTray_Dummy";

	if (!RegisterClassEx(&wcex)) 
		LOGI((L"[main.cpp] Tray class registration failed (" + std::to_wstring(GetLastError()) + L")").c_str());

	g_trayHwnd = CreateWindowEx
	(
		NULL,
		L"ScreenExTray_Dummy",
		L"",
		NULL,
		0,
		0,
		0,
		0,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);

	if(!g_trayHwnd)
		LOGI((L"[main.cpp] Tray window creation failed (" + std::to_wstring(GetLastError()) + L")").c_str());

	ZeroMemory(&g_niData, sizeof(NOTIFYICONDATA));

	g_niData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SNIPPLUSPLUS));
	g_niData.cbSize = sizeof(NOTIFYICONDATA);
	g_niData.uID = TRAY_ICON_ID;

	g_niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	g_niData.hWnd = g_trayHwnd;
	g_niData.uTimeout = 10000;
	g_niData.uVersion = NOTIFYICON_VERSION_4;
	g_niData.uCallbackMessage = WM_TRAY;
	wcscpy_s(g_niData.szTip, L"ScreenEx");

	Shell_NotifyIcon(NIM_ADD, &g_niData);
}

void DeregisterTray()
{
	DestroyWindow(g_trayHwnd);
	Shell_NotifyIcon(NIM_DELETE, &g_niData);
}

void LoadSettings()
{
	DWORD vType;
	DWORD vLen = 0;
	DWORD val = 0;
	if (RegQueryValueEx(g_settingsKey, L"SaveScreenshots", NULL, &vType, NULL, &vLen) == ERROR_SUCCESS && vType == REG_BINARY)
	{
		RegQueryValueExW(g_settingsKey, L"SaveScreenshots", NULL, &vType, (LPBYTE)&val, &vLen);
		g_Config.saveScreenshots = val == 1;
	}
	if (RegQueryValueEx(g_settingsKey, L"CopyToClipboard", NULL, &vType, NULL, &vLen) == ERROR_SUCCESS && vType == REG_BINARY)
	{
		RegQueryValueExW(g_settingsKey, L"CopyToClipboard", NULL, &vType, (LPBYTE)&val, &vLen);
		g_Config.copyToClipboard = val == 1;
	}
	if (RegQueryValueEx(g_settingsKey, L"StartWithWindows", NULL, &vType, NULL, &vLen) == ERROR_SUCCESS && vType == REG_BINARY)
	{
		RegQueryValueExW(g_settingsKey, L"StartWithWindows", NULL, &vType, (LPBYTE)&val, &vLen);
		g_Config.startWithWindows = val == 1;
	}
	if (RegQueryValueEx(g_settingsKey, L"SaveLogs", NULL, &vType, NULL, &vLen) == ERROR_SUCCESS && vType == REG_BINARY)
	{
		RegQueryValueExW(g_settingsKey, L"SaveLogs", NULL, &vType, (LPBYTE)&val, &vLen);
		g_Config.saveLogs = val == 1;
	}
}

void SaveSettings()
{
	RegSetValueEx(g_settingsKey, L"SaveScreenshots", NULL, REG_BINARY, (BYTE*)&g_Config.saveScreenshots, 1);
	RegSetValueEx(g_settingsKey, L"CopyToClipboard", NULL, REG_BINARY, (BYTE*)&g_Config.copyToClipboard, 1);
	RegSetValueEx(g_settingsKey, L"StartWithWindows", NULL, REG_BINARY, (BYTE*)&g_Config.startWithWindows, 1);
	RegSetValueEx(g_settingsKey, L"SaveLogs", NULL, REG_BINARY, (BYTE*)&g_Config.saveLogs, 1);
}

void StartCapture()
{
	if (g_captureStarted)
		return;

	g_captureStarted = TRUE;

	Bitmap* bitmap = Bitmap::CaptureScreen();

	if (g_window) 
	{
		delete g_window;
		g_window = NULL;
	}

	g_window = new CaptureWindow(bitmap, &g_captureStarted, &g_Config);
}

void ShowRightClickMenu(HWND hwnd)
{
	SetForegroundWindow(hwnd);

	POINT curPoint;
	GetCursorPos(&curPoint);

	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING | MF_DISABLED, NULL, (L"ScreenEx v" + g_verString).c_str());
	AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(hMenu, MF_STRING | (g_Config.saveScreenshots ? MF_CHECKED : NULL), IDM_SAVESCREENSHOTS, L"Save screenshots");
	AppendMenu(hMenu, MF_STRING | (g_Config.copyToClipboard ? MF_CHECKED : NULL), IDM_COPYTOCLIPBOARD, L"Copy to clipboard");
	AppendMenu(hMenu, MF_STRING | (g_Config.startWithWindows ? MF_CHECKED : NULL), IDM_STARTWITHWINDOWS, L"Start with windows");
	AppendMenu(hMenu, MF_STRING | (g_Config.saveLogs ? MF_CHECKED : NULL), IDM_SAVELOGS, L"Save logs");
	AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(hMenu, MF_STRING , IDM_ABOUT, L"About");
	AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");

	UINT_PTR clicked = TrackPopupMenu(
		hMenu,
		TPM_RETURNCMD | TPM_NONOTIFY,
		curPoint.x,
		curPoint.y,
		0,
		hwnd,
		NULL
	);

	if (clicked) {
		switch (clicked)
		{
		case IDM_SAVESCREENSHOTS:
			g_Config.saveScreenshots = !g_Config.saveScreenshots;
			break;
		case IDM_COPYTOCLIPBOARD:
			g_Config.copyToClipboard = !g_Config.copyToClipboard;
			break;
		case IDM_STARTWITHWINDOWS:
			g_Config.startWithWindows = !g_Config.startWithWindows;
			SetAutoStartup(g_Config.startWithWindows);
			break;
		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		case IDM_SAVELOGS:
			g_Config.saveLogs = !g_Config.saveLogs;
			Logger::Enabled = g_Config.saveLogs;
			break;
		case IDM_ABOUT:
			MessageBox(hwnd, (L"ScreenEx v" + g_verString + L"\nCreated by Murky\nBuilt at: " __DATE__ L" " __TIME__).c_str(), L"About ScreenEx", MB_OK | MB_ICONINFORMATION);
			break;
		default:
			break;
		}
	}

	SaveSettings();

	DestroyMenu(hMenu);
}

void SetAutoStartup(BOOL enabled)
{
	DWORD val = 0;
	DWORD valSize = sizeof(DWORD);
	DWORD valType = REG_NONE;
	HKEY runHkey = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &runHkey) != ERROR_SUCCESS)
	{
		RegCloseKey(runHkey);
		return;
	}
	if (enabled) {
		wchar_t buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);
		RegSetValueEx(runHkey, L"ScreenEx", 0, REG_SZ, reinterpret_cast<const BYTE*>(buffer), MAX_PATH);
	}
	else {
		RegDeleteValue(runHkey, L"ScreenEx");
	}
	RegCloseKey(runHkey);
}

static LRESULT CALLBACK TrayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_TRAY:
	{
		switch (lParam)
		{
		case WM_RBUTTONDOWN:
		{
			ShowRightClickMenu(hwnd);
			break;
		}
		}
		break;
	}
	default:
	{
		if (msg == WM_TASKBARCREATED)
			Shell_NotifyIcon(NIM_ADD, &g_niData);
	}
	break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (lParam != NULL)
	{
		switch (wParam)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			switch (((LPKBDLLHOOKSTRUCT)lParam)->vkCode)
			{
			case VK_S:
			{
				if (GetAsyncKeyState(VK_LMENU) < 0 && GetAsyncKeyState(VK_LCONTROL) < 0)
				{
					StartCapture();
					return TRUE;
				}

			}
			break;
			case VK_LCONTROL:
			{
				if (GetAsyncKeyState(VK_S) < 0 && GetAsyncKeyState(VK_LMENU) < 0)
				{
					StartCapture();
					return TRUE;
				}
			}
			break;
			case VK_LMENU:
			{
				if (GetAsyncKeyState(VK_S) < 0 && GetAsyncKeyState(VK_LCONTROL) < 0)
				{
					StartCapture();
					return TRUE;
				}
			}
			break;
			case VK_ESCAPE:
			{
				if (g_captureStarted) 
				{
					g_window->Close();
					return TRUE;
				}
			}
			break;
			default:
				break;
			}
			break;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}