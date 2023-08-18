#include "pch.h"
#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <tlhelp32.h>
#include "MinHook.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_win32.h"

// Our state
bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

DWORD WINAPI MainThread(HMODULE hModule);
void HookOpenGL();
BOOL APIENTRY h_wglSwapBuffers(HDC hdc);

typedef BOOL(APIENTRY* t_wglSwapBuffers)(HDC hdc);
t_wglSwapBuffers o_wglSwapBuffers;

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

ImGuiIO& createImGui(HWND hwnd) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    IMGUI_CHECKVERSION();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();
    return io;
}

void destroyImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

DWORD WINAPI MainThread(HMODULE hModule) {
    AllocConsole();
    freopen_s(&stream, "CONOUT$", "w", stdout);

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        std::cerr << "Failed to initialize MinHook! Error: " << MH_StatusToString(status) << std::endl;
        return 1;
    }
    std::cout << "MinHook initialized successfully!" << std::endl;

    DWORD moduleBaseAddress = GetModuleBaseAddress("ac_client.exe");
    const DWORD localPlayerOffset = 0x0017E0A8;
    DWORD* localPlayerBasePtr = (DWORD*)(moduleBaseAddress + localPlayerOffset);

    if (!localPlayerBasePtr) {
        std::cerr << "Failed to find localPlayerBasePtr." << std::endl;
        return 1;
    }

    PlayerEntity* player = (PlayerEntity*)*localPlayerBasePtr;
    if (!player) {
        std::cerr << "Failed to dereference local player." << std::endl;
        return 1;
    }

    std::cout << "LocalPlayer: 0x" << std::hex << std::uppercase << (uintptr_t)localPlayerBasePtr << std::endl;

    std::cout << "Localhealth: " << player->health << std::endl;
    std::cout << "LocalKevlarHealth: " << player->kevlarHealth << std::endl;
    std::cout << "LocalPrimaryAmmo: " << player->primaryAmmo << std::endl;
    std::cout << "LocalPrimarayAmmo2: " << player->primaryAmmo2 << std::endl;
    std::cout << "LocalSecondaryAmmo: " << player->secondaryAmmo << std::endl;
    std::cout << "LocalSecondaryAmmo2: " << player->secondaryAmmo2 << std::endl;
    std::cout << "LocalGranadeAmmo: " << player->grenadeAmmo << std::endl;

    // Create ImGui context
    /*/IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    // Init
    ImGui_ImplWin32_Init(GetActiveWindow());
    ImGui_ImplOpenGL3_Init();*/

    HookOpenGL();

    while (true) {
        if (GetAsyncKeyState(VK_F5) & 0x8000) {
            break;
        }
    }

    if (stream) {
        fclose(stream);
    }

    MH_Uninitialize();
    /*ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();*/

    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);

    return 0;
}

void HookOpenGL() {
    MH_STATUS status = MH_CreateHookApi(L"opengl32.dll", "wglSwapBuffers", &h_wglSwapBuffers, reinterpret_cast<LPVOID*>(&o_wglSwapBuffers));
    if (status == MH_OK) {
        std::cout << "[Hook Status] Original wglSwapBuffers at address: 0x"
            << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(o_wglSwapBuffers);
    }
    std::cout << " - Hook status: " << MH_StatusToString(status) << std::endl;
    MH_EnableHook(MH_ALL_HOOKS);
}



BOOL APIENTRY h_wglSwapBuffers(HDC hdc) {
    o_wglSwapBuffers(hdc);
    /*ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("______ Hack");
    ImGui::SetWindowSize(ImVec2(512, 128), ImGuiCond_Once);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());*/

    return true;
}