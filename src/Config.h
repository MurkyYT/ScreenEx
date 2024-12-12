#pragma once
#include <Windows.h>
struct Config
{
	BOOL saveScreenshots = TRUE;
	BOOL copyToClipboard = TRUE;
	BOOL startWithWindows = FALSE;
	BOOL saveLogs = FALSE;
};