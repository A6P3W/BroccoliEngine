#pragma once

#include <string>
#include <vector>

#include "ActorComponent.h"

class FSoundManager;

class MSoundComponent : public MActorComponent {
 public:
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

  std::vector<int> PlayingHandles;
};
