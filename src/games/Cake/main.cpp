
#include <windows.h>
#include <string>
#include <cstring>

#include "Cake.hpp"

int main() {

  std::cout << "Hiii" << std::endl;
  std::cout << "You're super amazing!! I hope you like the show :3" << std::endl;
  std::cout << "Meow and happy birthday!!! Listen to your favourite tracks!" << std::endl;

  Cake game;
  game.Run();
  return 0;
}

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

    Cake game;
    game.Run();
    game.ActivateScreenSaverMode(mode, previewHwnd);

    return 0;
}