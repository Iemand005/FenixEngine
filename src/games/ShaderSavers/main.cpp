#include "ShaderSaver.hpp"

int main(int argc, char* argv[]) {

	std::cout << "Hiii";

	for (int i = 0; i < argc; i++)
	{
		std::cout << "arg[" << i << "] = " << argv[i] << "\n";
	}

	// #ifdef WIN32

	ScreenSaverMode mode = ScreenSaverMode::Fullscreen;
	HWND previewHwnd = nullptr;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "/p") == 0 && i + 1 < argc)
		{
			mode = ScreenSaverMode::Preview;
			previewHwnd = (HWND)std::stoull(argv[i + 1]);
		}

		if (_stricmp(argv[i], "/s") == 0)
			mode = ScreenSaverMode::Fullscreen;

		if (_stricmp(argv[i], "/c") == 0)
        	mode = ScreenSaverMode::Config;
	}

	ShaderSaver game;
	game.Run(mode, previewHwnd);
	return 0;
}

#include <windows.h>
#include <string>

int main(int argc, char** argv);

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    int argc = 1;
    char* argv[10] = { nullptr };

    argv[0] = (char*)"ShaderSaver.scr";

    // Simple tokenization of lpCmdLine
    std::string cmd = lpCmdLine ? lpCmdLine : "";
    std::string current;

    for (char c : cmd)
    {
        if (c == ' ')
        {
            if (!current.empty())
            {
                argv[argc++] = _strdup(current.c_str());
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
        argv[argc++] = _strdup(current.c_str());

    return main(argc, argv);
}