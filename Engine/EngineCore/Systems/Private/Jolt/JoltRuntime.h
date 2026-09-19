#pragma once

class FJoltRuntime {
 public:
  static bool Initialize();
  static void Shutdown();
  static bool IsInitialized();
};
