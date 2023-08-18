// ACInjector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

bool FileExists(const char* filePath);

bool InjectDLL(DWORD processId, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, processId);

    if (!FileExists(dllPath)) {
        std::cerr << "The DLL does not exist at the specified path." << std::endl;
        return false;
    }

    if (!hProcess) {
        std::cerr << "Failed to open the process of a processId: " << processId << std::endl;
        return false;
    }

    size_t pathLength = strlen(dllPath) + 1;
    void* pDllPath = VirtualAllocEx(hProcess, NULL, pathLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!pDllPath) {
        std::cerr << "Failed to allocate memory in the target process." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    bool wpmResult = WriteProcessMemory(hProcess, pDllPath, dllPath, pathLength, NULL);

    if (!wpmResult) {
        std::cerr << "Failed to write to process memory." << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hToRThread = CreateRemoteThread(hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)LoadLibraryA,
        pDllPath, 0, NULL);

    if (!hToRThread) {
        std::cerr << "Failed to create a remote thread." << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    CloseHandle(hToRThread);
    CloseHandle(hProcess);
    return true;
}

DWORD GetProcessID(const char* processName) {
    DWORD processId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get process snapshot for: " << processName << std::endl;
        return 0;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnap, &pe32)) {
        std::cerr << "Failed to initialize process retrieval (for: " << processName << ")" << std::endl;
        CloseHandle(hSnap);
        return 0;
    }

    do {
        if (!_stricmp(pe32.szExeFile, processName)) {
            processId = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hSnap, &pe32));

    CloseHandle(hSnap);
    return processId;
}

bool FileExists(const char* filePath)
{
    DWORD fileAttr;

    fileAttr = GetFileAttributesA(filePath);
    if (fileAttr == INVALID_FILE_ATTRIBUTES)
    {
        return false;  // Something is wrong with your path!
    }

    if (fileAttr & FILE_ATTRIBUTE_DIRECTORY)
    {
        return false;  // This is a directory!
    }

    return true;  // File exists
}

int main()
{
    const char* targetProcessName = "ac_client.exe";

    DWORD processId = GetProcessID(targetProcessName);

    if (processId) {
        std::cout << "Process ID for " << targetProcessName << ": " << processId << std::endl;
    }
    else {
        std::cerr << "Failed to get process ID for " << targetProcessName << std::endl;
        return 1;
    }

    const char* dllPath = "C:\\Users\\shant\\Desktop\\AssaultCubePractice\\ACInternal\\Debug\\ACInternal.dll";

    if (InjectDLL(processId, dllPath)) {
        std::cout << "DLL injected successfully." << std::endl;
    }
    else {
        std::cerr << "DLL injection failed." << std::endl;
        return 1; 
    }

    Sleep(3000);

    return 0;
}
