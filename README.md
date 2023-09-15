# ACInternal
This repository contains an internal cheat for Assault Cube, leveraging MinHook to hook into the game's functions and display an ImGui overlay. This overlay provides real-time statistics of the local player, such as health, ammo, and more. For the injection mechanism, a LoadLibrary-style DLL injector is included to facilitate injecting the cheat into the Assault Cube process.

The primary purpose of this cheat is educational, aimed at demonstrating how to hook functions effectively and how to implement a real-time updating ImGui overlay.

![ac_client_8S5mRWlmVl](https://github.com/sneezingabbey/ACInternal/assets/61197745/adf25492-c24f-4ce1-b813-ec4f0d4c59b9)

## Prerequisites

Ensure you have the following installed:

- [Microsoft Visual C++ Redistributable](https://visualstudio.microsoft.com/visual-cpp-build-tools/)
- [DirectX End-User Runtimes](https://www.microsoft.com/en-us/download/details.aspx?id=35)

## Dependencies
- [ImGui](https://github.com/ocornut/imgui)
- [MinHook](https://github.com/TsudaKageyu/minhook)
  
## Educational Purposes Disclaimer
This repository is intended for educational purposes only. The content, code, and any associated materials are provided "as is" and without any express or implied warranties. 
