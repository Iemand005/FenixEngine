#define FE_EXCLUDE_GLFW
#include "ShaderSaver.hpp"

#include <windows.h>
#include <string>
#include <cstring>

ScreenSaverMode mode = ScreenSaverMode::Window;
HWND previewHwnd = nullptr;

static void ParseArgs(char* lpCmdLine)
{
    std::string cmd = lpCmdLine ? lpCmdLine : "";
    std::string cur;

    auto flush = [&]() {
        if (cur.empty()) return;

        if (_stricmp(cur.c_str(), "/s") == 0)
            mode = ScreenSaverMode::Fullscreen;

        else if (_stricmp(cur.c_str(), "/c") == 0)
            mode = ScreenSaverMode::Config;

        else if (_stricmp(cur.c_str(), "/p") == 0)
            mode = ScreenSaverMode::Preview;

        else if (mode == ScreenSaverMode::Preview && previewHwnd == nullptr)
            previewHwnd = (HWND)std::stoull(cur);

        cur.clear();
    };

    for (char c : cmd)
    {
        if (c == ' ')
            flush();
        else
            cur += c;
    }

    flush();
}

int WINAPI WinMain(
    HINSTANCE,
    HINSTANCE,
    LPSTR lpCmdLine,
    int
)
{
    ParseArgs(lpCmdLine);

    ShaderSaver game;
    game.Run(mode, previewHwnd);

    return 0;
}