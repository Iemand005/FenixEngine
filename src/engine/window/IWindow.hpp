
namespace fe {

  class IWindow {

    bool shouldClose = false;

public:

  virtual bool ShouldClose() { return shouldClose; }

  virtual void PrepareClose() { shouldClose = true; }

	virtual void SetSwapInterval(int interval) = 0;

  void EnableVSync() {
    SetSwapInterval(1);
  }

  void DisableVSync() {
    SetSwapInterval(0);
  }
  
  virtual void SwapBuffers() = 0;

  // virtual void PollEvents() = 0;

	virtual void Destroy() = 0;

  };
}