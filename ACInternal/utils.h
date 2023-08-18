#ifndef UTILS_H
#define UTILS_H

#include <windows.h>

namespace utils
{
    BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam);
    uintptr_t GetModuleBaseAddress(const char* moduleName);
    HWND GetProcessWindow();
}

#endif // UTILS_H
