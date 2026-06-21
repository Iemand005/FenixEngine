#define FE_USE_SDL
#include "ShaderSaver.hpp"

#include <windows.h>
#include <string>
#include <cstring>

int WINAPI WinMain(
    HINSTANCE,
    HINSTANCE,
    LPSTR lpCmdLine,
    int
)
{
    ScreenSaverMode mode = ScreenSaverMode::Window;
    HWND previewHwnd = nullptr;

    std::string cmd = lpCmdLine ? lpCmdLine : "";
    std::string cur;

    for (char c : cmd)
    {
        if (c == ' ')
        {
            if (!cur.empty())
            {
                if (_stricmp(cur.c_str(), "/s") == 0)
                    mode = ScreenSaverMode::Fullscreen;
                else if (_stricmp(cur.c_str(), "/c") == 0)
                    mode = ScreenSaverMode::Config;
                else if (_stricmp(cur.c_str(), "/p") == 0)
                    mode = ScreenSaverMode::Preview;
                else if (mode == ScreenSaverMode::Preview)
                    previewHwnd = (HWND)std::stoull(cur);

                cur.clear();
            }
        }
        else
        {
            cur += c;
        }
    }

    if (!cur.empty())
    {
        if (_stricmp(cur.c_str(), "/s") == 0)
            mode = ScreenSaverMode::Fullscreen;
        else if (_stricmp(cur.c_str(), "/c") == 0)
            mode = ScreenSaverMode::Config;
        else if (_stricmp(cur.c_str(), "/p") == 0)
            mode = ScreenSaverMode::Preview;
        else if (mode == ScreenSaverMode::Preview)
            previewHwnd = (HWND)std::stoull(cur);
    }

    ShaderSaver game;
    game.Run(mode, previewHwnd);

    return 0;
}