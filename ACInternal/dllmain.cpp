#include "pch.h"
#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <tlhelp32.h>
#include "MinHook.h"

DWORD WINAPI MainThread(HMODULE hModule);

FILE* stream;

BOOL APIENTRY DllMain( HMODULE hModule,
                        DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
 switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
        if (hThread == NULL) {
            // Handle error (for example, by logging or by showing a message box)
            // You might decide to return FALSE from DllMain to indicate the DLL failed to initialize properly
            return false;
        }
        else {
            CloseHandle(hThread);
        }
        break;
    }
    return TRUE;
}

struct PlayerEntity
{
    char pad_0000[236]; // Padding or unknown data
    int32_t health;     // Offset: 0x00EC
    int32_t kevlarHealth;     // Offset: 0x00F0
    char pad_00F4[20];  // Padding or unknown data
    int32_t secondaryAmmo2; // Offset: 0x0108
    char pad_010C[16];  // Padding or unknown data
    int32_t primaryAmmo2;   // Offset: 0x011C
    char pad_0120[12];  // Padding or unknown data
    int32_t secondaryAmmo;  // Offset: 0x012C
    char pad_0130[16];  // Padding or unknown data
    int32_t primaryAmmo;    // Offset: 0x0140
    int32_t grenadeAmmo;          // Offset: 0x0144
    char pad_0148[4088];          // Padding or unknown data until the end of the structure
};

DWORD GetModuleBaseAddress(const char* moduleName) {
    DWORD moduleBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnap, &moduleEntry)) {
        do {
            if (!_stricmp(moduleEntry.szModule, moduleName)) {
                moduleBaseAddr = (DWORD)moduleEntry.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnap, &moduleEntry));
    }
    CloseHandle(hSnap);
    return moduleBaseAddr;
}

DWORD WINAPI MainThread(HMODULE hModule) {
    AllocConsole();
    freopen_s(&stream, "CONOUT$", "w", stdout);
    std::cout << "We Can Use Console For Debugging!\n";

    MH_STATUS status = MH_Initialize();

    if (status != MH_OK)
    {
        std::cerr << "Failed to initialize MinHook! Error: " << MH_StatusToString(status) << std::endl;
        return 1;
    }

    std::cout << "MinHook initialized successfully!" << std::endl;

    // Assuming you have the moduleBaseAddress correctly retrieved:
    DWORD moduleBaseAddress = GetModuleBaseAddress("ac_client.exe");

    // The static offset to the local player entity:
    const DWORD localPlayerOffset = 0x0017E0A8;

    // Get the pointer stored at "ac_client.exe"+0017E0A8
    DWORD* localPlayerBasePtr = (DWORD*)(moduleBaseAddress + localPlayerOffset);

    // Dereference the pointer to get the actual player entity's address
    PlayerEntity* player = (PlayerEntity*)*localPlayerBasePtr;

    std::cout << "LocalPlayer: 0x" << std::hex << std::uppercase << (uintptr_t)localPlayerBasePtr << std::endl;

    std::cout << "Localhealth: " << player->health << std::endl;
    std::cout << "LocalKevlarHealth: " << player->kevlarHealth << std::endl;
    std::cout << "LocalPrimaryAmmo: " << player->primaryAmmo << std::endl;
    std::cout << "LocalPrimarayAmmo2: " << player->primaryAmmo2 << std::endl;
    std::cout << "LocalSecondaryAmmo: " << player->secondaryAmmo << std::endl;
    std::cout << "LocalSecondaryAmmo2: " << player->secondaryAmmo2 << std::endl;
    std::cout << "LocalGranadeAmmo: " << player->grenadeAmmo << std::endl;
    

    while (true) {
        if (GetAsyncKeyState(VK_F5) & 0x8000) {
            break;
        }
    }

    if (stream == NULL) return 0;

    MH_Uninitialize();

    fclose(stream);
    FreeConsole();

    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}