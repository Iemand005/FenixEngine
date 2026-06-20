#include "ShaderSaver.hpp"

int main(int argc, char* argv[]) {

  std::cout << "Hiii";

  for (int i = 0; i < argc; i++)
  {
    std::cout << "arg[" << i << "] = " << argv[i] << "\n";
  }

  ShaderSaver game;
  game.Run();
  return 0;
}