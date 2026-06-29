#pragma once
#include <memory>
#include <vector>
class AGameModeBase;
class Application {
 public:
  Application();
  ~Application();
  bool Run();

  static void SetWindowResolution(int width, int height);
  static void QuitGame();

 private:
  bool Update(float DeltaTime);
  bool Draw();

  float DeltaTime = 0.0f;
  bool bPosed = false;

  void InitOffscreenBuffer();
  int OffscreenBuffer = -1;
};
