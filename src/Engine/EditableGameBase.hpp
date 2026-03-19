
#include "XRGame.hpp"

namespace fe
{
  class EditableGameBase
  {
  private:
    std::unique_ptr<XRGame> game;
  public:
    EditableGameBase();
    ~EditableGameBase();
  };
  
  EditableGameBase::EditableGameBase()
  {
    
  }
  
  EditableGameBase::~EditableGameBase()
  {
  }
  
} // namespace fe
