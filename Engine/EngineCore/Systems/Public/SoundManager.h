#pragma once

#include "BroccoliEngineAPI.h"
#include "FPath.h"

class BROCCOLI_ENGINE_API FSoundManager {
 public:
  FSoundManager();
  ~FSoundManager();

  FSoundManager(const FSoundManager&) = delete;
  FSoundManager& operator=(const FSoundManager&) = delete;

  int GetMasterHandle(const FPath& Path);
  int PlaySE(const FPath& Path, bool Loop = false);
  int PlayBGM(const FPath& Path, bool Loop = true);

  void Update();
  void SetVolume(int Handle, float Volume);
  void Stop(int Handle);
  void SetMasterVolume(float Volume);

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
