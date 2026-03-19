
#include "XRGame.hpp"

namespace fe
{
  class GameEditor
  {
  private:
    std::unique_ptr<XRGame> game;
  public:
    GameEditor();
    ~GameEditor();
  };
  
  GameEditor::GameEditor()
  {
    
  }
  
  GameEditor::~GameEditor()
  {
  }
  
} // namespace fe
