#include "ShaderSaver.hpp"

int main(int argc, char* argv[]) {

	std::cout << "Hiii";

	for (int i = 0; i < argc; i++)
	{
		std::cout << "arg[" << i << "] = " << argv[i] << "\n";
	}

	// #ifdef WIN32

	bool isPreview = false;
	bool screenSaverMode = false;
	HWND previewHwnd = nullptr;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "/p") == 0 && i + 1 < argc)
		{
			isPreview = true;
			previewHwnd = (HWND)std::stoull(argv[i + 1]);
		}

		if (_stricmp(argv[i], "/s") == 0)
			screenSaverMode = true;
	}

	ShaderSaver game;
	game.Run(isPreview, previewHwnd);
	return 0;
}