#include "WindowsUtils.h"
std::wstring GetCurrentDir()
{
	WCHAR buffer[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	std::wstring path = std::wstring(buffer);
	std::wstring::size_type pos = path.find_last_of(L"\\/");
	return path.substr(0, pos);
}
BOOL FileExists(const char* name)
{
	return _access_s(name, 0) != ENOENT;
}
BOOL FileExists(const wchar_t* name)
{
	return _waccess_s(name, 0) != ENOENT;
}
BOOL DirectoryExists(const wchar_t* szPath)
{
	DWORD dwAttrib = GetFileAttributesW(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
BOOL DirectoryExists(const char* szPath)
{
	DWORD dwAttrib = GetFileAttributesA(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}