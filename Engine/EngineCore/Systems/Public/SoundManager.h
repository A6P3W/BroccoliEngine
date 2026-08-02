#pragma once

#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API FSoundManager {
 public:
  FSoundManager();
  ~FSoundManager();

  FSoundManager(const FSoundManager&) = delete;
  FSoundManager& operator=(const FSoundManager&) = delete;

  int GetMasterHandle(const std::string& Path);
  int PlaySE(const std::string& Path, bool Loop = false);
  int PlayBGM(const std::string& Path, bool Loop = true);

  void Update();
  void SetVolume(int Handle, float Volume);
  void Stop(int Handle);
  void SetMasterVolume(float Volume);

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
