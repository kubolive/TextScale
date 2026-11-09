#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include "payload.h"

static bool IsOBSStudioRunning()
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE)
		return false;

	PROCESSENTRY32W pe;
	pe.dwSize = sizeof(pe);
	bool running = false;

	if (Process32FirstW(snap, &pe)) {
		do {
			if (_wcsicmp(pe.szExeFile, L"obs64.exe") == 0 ||
			    _wcsicmp(pe.szExeFile, L"obs32.exe") == 0 ||
			    _wcsicmp(pe.szExeFile, L"obs.exe") == 0) {
				running = true;
				break;
			}
		} while (Process32NextW(snap, &pe));
	}

	CloseHandle(snap);
	return running;
}

static bool CreateDirectoriesRecursively(const std::wstring &path)
{
	wchar_t folder[MAX_PATH];
	wcsncpy(folder, path.c_str(), MAX_PATH);
	wchar_t *end = folder;

	while (*end != 0) {
		if (*end == L'\\' || *end == L'/') {
			wchar_t orig = *end;
			*end = 0;
			CreateDirectoryW(folder, NULL);
			*end = orig;
		}
		end++;
	}
	return CreateDirectoryW(folder, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static bool WriteEmbeddedFile(const std::wstring &dest_path, const unsigned char *data, size_t size)
{
	HANDLE hFile = CreateFileW(dest_path.c_str(), GENERIC_WRITE, 0, NULL,
				   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD written = 0;
	BOOL ok = WriteFile(hFile, data, (DWORD)size, &written, NULL);
	CloseHandle(hFile);
	return ok && (written == size);
}

static bool InstallToDirectory(const std::wstring &base_dir)
{
	std::wstring bin_dir = base_dir + L"\\bin\\64bit";
	std::wstring data_dir = base_dir + L"\\data";
	std::wstring locale_dir = base_dir + L"\\data\\locale";

	if (!CreateDirectoriesRecursively(bin_dir) ||
	    !CreateDirectoriesRecursively(data_dir) ||
	    !CreateDirectoriesRecursively(locale_dir)) {
		return false;
	}

	bool ok = true;
	ok &= WriteEmbeddedFile(bin_dir + L"\\text-scale.dll", PAYLOAD_DLL_data, PAYLOAD_DLL_len);
	if (PAYLOAD_PTHREAD_len > 0) {
		ok &= WriteEmbeddedFile(bin_dir + L"\\libwinpthread-1.dll", PAYLOAD_PTHREAD_data, PAYLOAD_PTHREAD_len);
	}
	ok &= WriteEmbeddedFile(data_dir + L"\\text_scale.effect", PAYLOAD_EFFECT_data, PAYLOAD_EFFECT_len);
	ok &= WriteEmbeddedFile(locale_dir + L"\\en-US.ini", PAYLOAD_LOCALE_data, PAYLOAD_LOCALE_len);

	// Write uninstaller batch script
	std::string uninstaller_script = 
		"@echo off\r\n"
		"title Uninstall TextScale Plugin for OBS Studio\r\n"
		"echo Uninstalling TextScale Plugin...\r\n"
		"taskkill /F /IM obs64.exe >nul 2>&1\r\n"
		"taskkill /F /IM obs32.exe >nul 2>&1\r\n"
		"timeout /t 1 >nul\r\n"
		"rmdir /s /q \"%~dp0\\bin\" 2>nul\r\n"
		"rmdir /s /q \"%~dp0\\data\" 2>nul\r\n"
		"echo TextScale Plugin has been successfully uninstalled!\r\n"
		"pause\r\n";
	
	std::wstring uninstaller_path = base_dir + L"\\Uninstall_TextScale.bat";
	HANDLE hUninst = CreateFileW(uninstaller_path.c_str(), GENERIC_WRITE, 0, NULL,
				     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hUninst != INVALID_HANDLE_VALUE) {
		DWORD written = 0;
		WriteFile(hUninst, uninstaller_script.c_str(), (DWORD)uninstaller_script.length(), &written, NULL);
		CloseHandle(hUninst);
	}

	return ok;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	(void)hInstance;
	(void)hPrevInstance;
	(void)nCmdShow;

	bool silent = false;
	if (strstr(lpCmdLine, "/S") || strstr(lpCmdLine, "/s") || strstr(lpCmdLine, "--silent")) {
		silent = true;
	}

	// 1. Check if OBS is running
	if (IsOBSStudioRunning()) {
		if (!silent) {
			int choice = MessageBoxW(
				NULL,
				L"OBS Studio is currently running!\n\n"
				L"To avoid file write conflicts, please close OBS Studio before proceeding.\n\n"
				L"Do you still want to continue the installation?",
				L"TextScale Setup - Warning",
				MB_YESNO | MB_ICONWARNING
			);
			if (choice != IDYES) {
				return 1;
			}
		}
	}

	// 2. Resolve target directories
	wchar_t appdata[MAX_PATH];
	if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
		if (!silent) {
			MessageBoxW(NULL, L"Unable to locate system %APPDATA% directory!",
				    L"TextScale Setup - Error", MB_OK | MB_ICONERROR);
		}
		return 2;
	}

	std::wstring user_plugin_dir = std::wstring(appdata) + L"\\obs-studio\\plugins\\text-scale";
	std::wstring programdata_dir = L"C:\\ProgramData\\obs-studio\\plugins\\text-scale";

	// 3. Install
	bool user_ok = InstallToDirectory(user_plugin_dir);
	InstallToDirectory(programdata_dir); // Also install system-wide if accessible

	if (!user_ok) {
		if (!silent) {
			MessageBoxW(
				NULL,
				L"Failed to write plugin files to the OBS plugins directory.\n"
				L"Please try running the installer as Administrator!",
				L"TextScale Setup - Error",
				MB_OK | MB_ICONERROR
			);
		}
		return 3;
	}

	// 4. Success message
	if (!silent) {
		std::wstring msg =
			L"TextScale Plugin for OBS Studio installed successfully!\n\n"
			L"Installation Directory:\n" + user_plugin_dir + L"\n\n"
			L"QUICK START GUIDE:\n"
			L"1. Launch OBS Studio.\n"
			L"2. Select any 'Text (GDI+)' source.\n"
			L"3. Right-click the source -> select Filters.\n"
			L"4. Under Effect Filters, click '+' -> choose 'TextScale'.\n"
			L"5. Enable 'Auto Condense' and set your Target Width to automatically\n"
			L"   condense headlines without reducing font size!\n\n"
			L"Happy streaming!";

		MessageBoxW(NULL, msg.c_str(), L"TextScale Setup - Completed", MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}
