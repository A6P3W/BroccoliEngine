#pragma once
#include "ActorComponent.h"
#include "BroccoliEngineAPI.h"
#include "FPath.h"

class FSoundManager;

class BROCCOLI_ENGINE_API MSoundComponent : public MActorComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MSoundComponent)
  MSoundComponent();
  ~MSoundComponent() override;

  int PlaySE(const FPath& Path, bool Loop = false);
  int PlayBGM(const FPath& Path, bool Loop = true);

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
