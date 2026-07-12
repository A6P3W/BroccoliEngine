#pragma once
#include "BroccoliEngineAPI.h"

#include <string>

#include "ActorComponent.h"

class FSoundManager;

class BROCCOLI_ENGINE_API MSoundComponent : public MActorComponent {
 public:
  MSoundComponent();
  ~MSoundComponent() override;

  int PlaySE(const std::string& path, bool loop = false);
  int PlayBGM(const std::string& path, bool loop = true);

  void SetVolume(int handle, float volume);
  void Stop(int handle);
  void StopAll();
  void SetMasterVolume(float volume);

 protected:
  void OnComponentDestroy() override;

 private:
  FSoundManager* GetSoundManager() const;

  struct Impl;
  Impl* ImplPtr = nullptr;
};