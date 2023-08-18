#include "pch.h"
#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <tlhelp32.h>

#include "utils.h"

#include "MinHook.h"
#if _WIN64
#pragma comment (lib, "libMinHook.x64.lib")
#else
#pragma comment (lib, "libMinHook.x86.lib")
#endif

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_opengl3_loader.h"

// Our state
bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

DWORD WINAPI MainThread(HMODULE hModule);

// Function signature of wglSwapLayerBuffers
typedef BOOL(WINAPI* WGLSWAPLAYERBUFFERS)(HDC);

BOOL WINAPI DetourwglSwapLayerBuffers(HDC hdc);

// Pointers for the original and hooked functions
WGLSWAPLAYERBUFFERS fpOriginalwglSwapLayerBuffers = NULL;

void HookOpenGL();

FILE* stream;
FILE* stream_err;

BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
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

//https://youtu.be/JHpauVqy064?t=483
//Reclass address: [<ac_client.exe>+00195404]
struct PlayerEntity
{
    float posX; //0x0000
    float posY; //0x0004
    float posZ; //0x0008
    char pad_000C[224]; //0x000C
    int32_t health; //0x00EC
    int32_t kevlarHealth; //0x00F0
    char pad_00F4[20]; //0x00F4
    int32_t secondaryAmmo2; //0x0108
    char pad_010C[16]; //0x010C
    int32_t primaryAmmo2; //0x011C
    char pad_0120[12]; //0x0120
    int32_t secondaryAmmo; //0x012C
    char pad_0130[16]; //0x0130
    int32_t primaryAmmo; //0x0140
    int32_t grenadeAmmo; //0x0144
    char pad_0148[3832]; //0x0148
};

// When you create a new thread using the Windows API (e.g., CreateThread or _beginthreadex), 
// the conventional return type for the thread function is DWORD. 
// This is reflected in the prototype for the thread function given in Microsoft's documentation.
DWORD WINAPI MainThread(HMODULE hModule) {
    AllocConsole();
    freopen_s(&stream, "CONOUT$", "w", stdout);
    // Redirect stderr
    freopen_s(&stream_err, "CONOUT$", "w", stderr);

    uintptr_t moduleBaseAddress = utils::GetModuleBaseAddress("ac_client.exe");
    const uintptr_t localPlayerOffset = 0x00195404;

    uintptr_t* localPlayerPointer = (uintptr_t*)(moduleBaseAddress + localPlayerOffset);
    
    if (!localPlayerPointer) {
        std::cerr << "Failed to get localPlayerPointer." << std::endl;
        return 1;
    }

    uintptr_t localPlayerAddress = *localPlayerPointer;

    if (!localPlayerAddress) {
        std::cerr << "Failed to get localPlayerAddress." << std::endl;
        return 1;
    }

    // Casts the memory address 'localPlayerAddress' to a pointer of type 'PlayerEntity'.
    // This means we are telling the compiler to treat the memory at 'localPlayerAddress' 
    // as if it holds a 'PlayerEntity' object. The 'player' pointer now points to this location.
    PlayerEntity* player = (PlayerEntity*)localPlayerAddress;
    if (!player) {
        std::cerr << "Failed to create a PlayerEntity that points at localPlayerAddress" << std::endl;
        return 1;
    }

    std::cout << "LocalPlayer: 0x" << localPlayerAddress << std::endl;
    std::cout << "LocalPosX: " << player->posX << std::endl;
    std::cout << "LocalPosY: " << player->posY << std::endl;
    std::cout << "LocalPosZ: " << player->posZ << std::endl;
    std::cout << "Localhealth: " << player->health << std::endl;
    std::cout << "LocalKevlarHealth: " << player->kevlarHealth << std::endl;
    std::cout << "LocalPrimaryAmmo: " << player->primaryAmmo << std::endl;
    std::cout << "LocalPrimarayAmmo2: " << player->primaryAmmo2 << std::endl;
    std::cout << "LocalSecondaryAmmo: " << player->secondaryAmmo << std::endl;
    std::cout << "LocalSecondaryAmmo2: " << player->secondaryAmmo2 << std::endl;
    std::cout << "LocalGrenadeAmmo: " << player->grenadeAmmo << std::endl;

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        std::cerr << "Failed to initialize MinHook! Error: " << MH_StatusToString(status) << std::endl;
    }
    std::cout << "MinHook initialized successfully!" << std::endl;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(utils::GetProcessWindow());

    HookOpenGL();

    while (true) {
        if (GetAsyncKeyState(VK_F5) & 0x8000) {
            break;
        }
    }

    if (stream) fclose(stream);
    if(stream_err) fclose(stream_err);

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);

    return 0;
}

void HookOpenGL() {
    HMODULE hOpenGL = GetModuleHandle("opengl32.dll");
    if (hOpenGL) {

        void* originalWglSwapLayersBuffers = GetProcAddress(hOpenGL, "wglSwapBuffers");

        MH_STATUS status = MH_CreateHook(originalWglSwapLayersBuffers, &DetourwglSwapLayerBuffers, (LPVOID*)&fpOriginalwglSwapLayerBuffers);
        if (status != MH_OK) {
            std::cerr << "Failed to create the hook: " << MH_StatusToString(status) << std::endl;
        }

        MH_STATUS enableStatus = MH_EnableHook(originalWglSwapLayersBuffers);
        if (enableStatus != MH_OK) {
            std::cerr << "Failed to enable the hook: " << MH_StatusToString(status) << std::endl;
        }
        else {
            std::cout << "wglSwapBuffers address: 0x" << &originalWglSwapLayersBuffers << std::endl;
        }
    }
    else {
        std::cerr << "Failed to get module handle for opengl32.dll!" << std::endl;
    }
}

BOOL WINAPI DetourwglSwapLayerBuffers(HDC hdc) {
    if (ImGui::GetCurrentContext()) {
        if (!ImGui::GetIO().BackendRendererUserData)
            ImGui_ImplOpenGL3_Init();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    return fpOriginalwglSwapLayerBuffers(hdc);
}