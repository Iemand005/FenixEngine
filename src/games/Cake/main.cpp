#define _WINSOCKAPI_
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <cstring>
#include <iostream>
#include "Cake.hpp"

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
                {
                    // Parse the window handle safely
                    try
                    {
                        unsigned long long handle = std::stoull(cur);
                        previewHwnd = (HWND)handle;
                    }
                    catch (const std::exception& e)
                    {
                        OutputDebugStringA("Failed to parse preview window handle: ");
                        OutputDebugStringA(e.what());
                        OutputDebugStringA("\n");
                    }
                }
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
        {
            try
            {
                unsigned long long handle = std::stoull(cur);
                previewHwnd = (HWND)handle;
            }
            catch (const std::exception& e)
            {
                OutputDebugStringA("Failed to parse preview window handle: ");
                OutputDebugStringA(e.what());
                OutputDebugStringA("\n");
            }
        }
    }
    
    Cake game;
    game.ActivateScreenSaverMode(mode, previewHwnd);
    game.Run();
    
    return 0;
}