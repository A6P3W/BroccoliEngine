#include "SoundComponent.h"

#include <algorithm>
#include <vector>

#include "Actor.h"
#include "SoundManager.h"
#include "World.h"

struct MSoundComponent::Impl {
  std::vector<int> PlayingHandles;
};

MSoundComponent::MSoundComponent() : ImplPtr(new Impl()) {}

MSoundComponent::~MSoundComponent() { delete ImplPtr; }

int MSoundComponent::PlaySE(const FPath& Path, bool Loop) {
  FSoundManager* SoundManager = GetSoundManager();
  if (!SoundManager) return -1;

  int Handle = SoundManager->PlaySE(Path, Loop);
  if (Handle != -1) {
    ImplPtr->PlayingHandles.push_back(Handle);
  }

  return Handle;
}

int MSoundComponent::PlayBGM(const FPath& Path, bool Loop) {
  FSoundManager* SoundManager = GetSoundManager();
  if (!SoundManager) return -1;

  int Handle = SoundManager->PlayBGM(Path, Loop);
  if (Handle != -1) {
    ImplPtr->PlayingHandles.push_back(Handle);
  }

  return Handle;
}

void MSoundComponent::SetVolume(int handle, float volume) {
  FSoundManager* soundManager = GetSoundManager();
  if (!soundManager) return;

  soundManager->SetVolume(handle, volume);
}

void MSoundComponent::Stop(int handle) {
  FSoundManager* soundManager = GetSoundManager();
  if (!soundManager) return;

  soundManager->Stop(handle);
  std::erase(ImplPtr->PlayingHandles, handle);
}

void MSoundComponent::StopAll() {
  FSoundManager* soundManager = GetSoundManager();
  if (!soundManager) {
    ImplPtr->PlayingHandles.clear();
    return;
  }

  for (int handle : ImplPtr->PlayingHandles) {
    soundManager->Stop(handle);
  }
  ImplPtr->PlayingHandles.clear();
}

void MSoundComponent::SetMasterVolume(float volume) {
  FSoundManager* soundManager = GetSoundManager();
  if (!soundManager) return;

  soundManager->SetMasterVolume(volume);
}

void MSoundComponent::OnComponentDestroy() { StopAll(); }

FSoundManager* MSoundComponent::GetSoundManager() const {
  if (!GetOwner() || !GetOwner()->GetWorld()) return nullptr;

  return GetOwner()->GetWorld()->GetSoundManager();
}
