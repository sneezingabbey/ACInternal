#include <Windows.h>
#include <thread>
#include <dxgi.h>
#include <TlHelp32.h>
#include <iostream>

namespace utils {

	BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
		const auto isMainWindow = [handle]() {
			return GetWindow(handle, GW_OWNER) == nullptr && IsWindowVisible(handle);
			};

		DWORD pID = 0;
		GetWindowThreadProcessId(handle, &pID);

		if (GetCurrentProcessId() != pID || !isMainWindow() || handle == GetConsoleWindow())
			return TRUE;

		*reinterpret_cast<HWND*>(lParam) = handle;

		return FALSE;
	}

	uintptr_t GetModuleBaseAddress(const char* moduleName) {
        uintptr_t moduleBaseAddr = 0;
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
        MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(MODULEENTRY32);

        if (Module32First(hSnap, &moduleEntry)) {
            do {
                if (!_stricmp(moduleEntry.szModule, moduleName)) {
                    moduleBaseAddr = (uintptr_t)moduleEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &moduleEntry));
        }
        CloseHandle(hSnap);
        return moduleBaseAddr;
    }


	HWND GetProcessWindow() {
		HWND hwnd = nullptr;
		EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&hwnd));

		char name[128];
		GetWindowTextA(hwnd, name, RTL_NUMBER_OF(name));
		std::cout << "GetProcessWindow: " << name << std::endl;

		return hwnd;
	}

}