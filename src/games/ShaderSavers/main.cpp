#include "ShaderSaver.hpp"

int main(int argc, char* argv[]) {

  std::cout << "Hiii";

  for (int i = 0; i < argc; i++)
  {
    std::cout << "arg[" << i << "] = " << argv[i] << "\n";
  }

  bool isPreview = false;
  HWND previewHwnd = nullptr;

  for (int i = 1; i < argc; i++)
  {
      if (strcmp(argv[i], "/p") == 0 && i + 1 < argc)
      {
          isPreview = true;
          previewHwnd = (HWND)std::stoull(argv[i + 1]);
      }
  }

  ShaderSaver game;
  game.Run();
  return 0;
}