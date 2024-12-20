#include <time.h>
#include <iostream>
#include "Logger.h"
#include "WindowsUtils.h"

#ifndef NDEBUG
LogLevel Logger::Level = DEBUG;
#else
LogLevel Logger::Level = INFO;
#endif
BOOL Logger::Enabled = FALSE;
BOOL Logger::WriteDebug(const wchar_t* text)
{
	if (Level == INFO)
		return FALSE;
	else if(Level == VERBOSE)
		WriteInfo(text);
	else
	{
		std::wstring result = std::wstring(text);
		if (result[result.size() - 1] != '\n')
			result.append(L"\n");
		wprintf(result.c_str());
		OutputDebugStringW(result.c_str());
	}
	return FALSE;
}
void Logger::GetTime(wchar_t* buffer,size_t size)
{
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	wcsftime(buffer, size, L"[%d.%m.%Y %H:%M:%S]", &timeinfo);
}
BOOL Logger::WriteInfo(const wchar_t* text)
{
	wchar_t timeBuffer[24];
	GetTime(timeBuffer, 24);

	std::wstring result = std::wstring(timeBuffer) + L" " + text;
	if (result[result.size()-1] != '\n')
		result.append(L"\n");
	if(Level != DEBUG && Logger::Enabled)
		return Write(result.c_str());
	return WriteDebug(text);
}
BOOL Logger::Write(const wchar_t* text)
{
	FILE* logFile = NULL;
	std::wstring wPath = GetCurrentDir() + L"\\log.txt";
	if (!FileExists(wPath.c_str()))
		_wfopen_s(&logFile, wPath.c_str(), L"w+ ,ccs=UTF-16LE");
	else
		_wfopen_s(&logFile, wPath.c_str(), L"a+ ,ccs=UTF-16LE");
	if (logFile == NULL)
		return FALSE;
	int success = fputws(text,logFile);
	fclose(logFile);
	return success == 0;
}