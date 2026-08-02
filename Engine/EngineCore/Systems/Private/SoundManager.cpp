#include "SoundManager.h"

#include <algorithm>
#include <optional>
#include <unordered_map>

#include "BroccoliRaylib.h"

namespace {
constexpr int InvalidSoundHandle = 0;

struct FPlayingSound {
  Sound Alias{};
  float Volume = 1.0f;
  bool Loop = false;
};

struct FPlayingMusic {
  Music Stream{};
  int Handle = InvalidSoundHandle;
  float Volume = 1.0f;
};
}  // namespace

struct FSoundManager::Impl {
  std::unordered_map<std::string, int> MasterSoundPaths;
  std::unordered_map<int, Sound> MasterSounds;
  std::unordered_map<int, FPlayingSound> PlayingSounds;
  std::optional<FPlayingMusic> PlayingMusic;
  int NextHandle = 1;
  float MasterVolume = 1.0f;
};

FSoundManager::FSoundManager() : ImplPtr(new Impl()) {}

FSoundManager::~FSoundManager() {
  if (IsAudioDeviceReady()) {
    if (ImplPtr->PlayingMusic.has_value()) {
      StopMusicStream(ImplPtr->PlayingMusic->Stream);
      UnloadMusicStream(ImplPtr->PlayingMusic->Stream);
    }
    for (auto& [Handle, PlayingSound] : ImplPtr->PlayingSounds) {
      StopSound(PlayingSound.Alias);
      UnloadSoundAlias(PlayingSound.Alias);
    }
    for (auto& [Handle, MasterSound] : ImplPtr->MasterSounds) {
      UnloadSound(MasterSound);
    }
  }
  delete ImplPtr;
}

int FSoundManager::GetMasterHandle(const std::string& Path) {
  const auto Cached = ImplPtr->MasterSoundPaths.find(Path);
  if (Cached != ImplPtr->MasterSoundPaths.end()) return Cached->second;
  if (!IsAudioDeviceReady()) return InvalidSoundHandle;

  const Sound MasterSound = LoadSound(Path.c_str());
  if (!IsSoundValid(MasterSound)) return InvalidSoundHandle;

  const int Handle = ImplPtr->NextHandle++;
  ImplPtr->MasterSounds.emplace(Handle, MasterSound);
  ImplPtr->MasterSoundPaths.emplace(Path, Handle);
  return Handle;
}

int FSoundManager::PlaySE(const std::string& Path, bool Loop) {
  const int MasterHandle = GetMasterHandle(Path);
  const auto MasterIt = ImplPtr->MasterSounds.find(MasterHandle);
  if (MasterIt == ImplPtr->MasterSounds.end()) return InvalidSoundHandle;

  const Sound Alias = LoadSoundAlias(MasterIt->second);
  if (!IsSoundValid(Alias)) return InvalidSoundHandle;

  const int Handle = ImplPtr->NextHandle++;
  FPlayingSound PlayingSound{Alias, 1.0f, Loop};
  SetSoundVolume(PlayingSound.Alias, ImplPtr->MasterVolume);
  PlaySound(PlayingSound.Alias);
  ImplPtr->PlayingSounds.emplace(Handle, PlayingSound);
  return Handle;
}

int FSoundManager::PlayBGM(const std::string& Path, bool Loop) {
  if (!IsAudioDeviceReady()) return InvalidSoundHandle;
  if (ImplPtr->PlayingMusic.has_value()) Stop(ImplPtr->PlayingMusic->Handle);

  Music Stream = LoadMusicStream(Path.c_str());
  if (!IsMusicValid(Stream)) return InvalidSoundHandle;

  Stream.looping = Loop;
  const int Handle = ImplPtr->NextHandle++;
  SetMusicVolume(Stream, ImplPtr->MasterVolume);
  PlayMusicStream(Stream);
  ImplPtr->PlayingMusic = FPlayingMusic{Stream, Handle, 1.0f};
  return Handle;
}

void FSoundManager::Update() {
  if (!IsAudioDeviceReady()) return;

  if (ImplPtr->PlayingMusic.has_value()) {
    UpdateMusicStream(ImplPtr->PlayingMusic->Stream);
  }

  for (auto It = ImplPtr->PlayingSounds.begin(); It != ImplPtr->PlayingSounds.end();) {
    FPlayingSound& PlayingSound = It->second;
    if (PlayingSound.Loop && !IsSoundPlaying(PlayingSound.Alias)) {
      PlaySound(PlayingSound.Alias);
      ++It;
    } else if (!PlayingSound.Loop && !IsSoundPlaying(PlayingSound.Alias)) {
      UnloadSoundAlias(PlayingSound.Alias);
      It = ImplPtr->PlayingSounds.erase(It);
    } else {
      ++It;
    }
  }
}

void FSoundManager::SetVolume(int Handle, float Volume) {
  const float ClampedVolume = std::clamp(Volume, 0.0f, 1.0f);
  const float EffectiveVolume = ClampedVolume * ImplPtr->MasterVolume;

  const auto SoundIt = ImplPtr->PlayingSounds.find(Handle);
  if (SoundIt != ImplPtr->PlayingSounds.end()) {
    SoundIt->second.Volume = ClampedVolume;
    SetSoundVolume(SoundIt->second.Alias, EffectiveVolume);
    return;
  }

  if (ImplPtr->PlayingMusic.has_value() && ImplPtr->PlayingMusic->Handle == Handle) {
    ImplPtr->PlayingMusic->Volume = ClampedVolume;
    SetMusicVolume(ImplPtr->PlayingMusic->Stream, EffectiveVolume);
  }
}

void FSoundManager::Stop(int Handle) {
  const auto SoundIt = ImplPtr->PlayingSounds.find(Handle);
  if (SoundIt != ImplPtr->PlayingSounds.end()) {
    StopSound(SoundIt->second.Alias);
    UnloadSoundAlias(SoundIt->second.Alias);
    ImplPtr->PlayingSounds.erase(SoundIt);
    return;
  }

  if (ImplPtr->PlayingMusic.has_value() && ImplPtr->PlayingMusic->Handle == Handle) {
    StopMusicStream(ImplPtr->PlayingMusic->Stream);
    UnloadMusicStream(ImplPtr->PlayingMusic->Stream);
    ImplPtr->PlayingMusic.reset();
  }
}

void FSoundManager::SetMasterVolume(float Volume) {
  ImplPtr->MasterVolume = std::clamp(Volume, 0.0f, 1.0f);
  for (auto& [Handle, PlayingSound] : ImplPtr->PlayingSounds) {
    SetSoundVolume(PlayingSound.Alias, PlayingSound.Volume * ImplPtr->MasterVolume);
  }
  if (ImplPtr->PlayingMusic.has_value()) {
    SetMusicVolume(
        ImplPtr->PlayingMusic->Stream, ImplPtr->PlayingMusic->Volume * ImplPtr->MasterVolume
    );
  }
}
