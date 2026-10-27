#include "../include/engine_sim_application.h"

#include <iostream>
#include <sstream>
#include <filesystem>

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    (void)nCmdShow;
#ifdef ATG_ENGINE_SIM_PORTABLE_RELEASE
    // Portable layout: exe/config/assets/es at the root, transient files in
    // runtime/, saved settings in workspace/. Never depend on Explorer's CWD.
    wchar_t filename[32768];
    const DWORD length = GetModuleFileNameW(nullptr, filename, 32768);
    if (length == 0 || length >= 32768) return 2;
    try {
        const auto runtime = std::filesystem::path(filename).parent_path() / L"runtime";
        std::filesystem::create_directories(runtime);
        std::filesystem::current_path(runtime);
    }
    catch (const std::filesystem::filesystem_error &) {
        MessageBoxW(nullptr, L"Extract the complete ZIP to a writable folder before running EngineSim.exe.",
            L"Engine Sim Lab", MB_OK | MB_ICONERROR);
        return 2;
    }
#endif
    std::istringstream arguments(lpCmdLine);
    std::string command;
    arguments >> command;
    if (command == "--validate-catalog") {
        return EngineSimApplication::validateEngineCatalog() ? 0 : 1;
    }
    (void)hPrevInstance;

    EngineSimApplication application;
    application.initialize((void *)&hInstance, ysContextObject::DeviceAPI::DirectX11);
    if (command == "--validate-history-ui") {
        const bool passed = application.validateHistoryInterface();
        application.destroy();
        return passed ? 0 : 1;
    }
    application.run();
    application.destroy();

    return 0;
}
