#include "SoundManager.h"

#include <DxLib.h>

#include <algorithm>

FSoundManager::~FSoundManager() {
  // 複製再生用ハンドルを全て強制終了・解放
  for (int handle : PlayHandles) {
    StopSoundMem(handle);
    DeleteSoundMem(handle);
  }
  PlayHandles.clear();

  for (const auto& sound : MasterSounds) {
    DeleteSoundMem(sound.second);
  }
}

int FSoundManager::GetMasterHandle(const std::string& path) {
  if (MasterSounds.count(path)) return MasterSounds[path];

  int handle = LoadSoundMem(path.c_str());
  if (handle == -1) return -1;

  MasterSounds[path] = handle;
  return handle;
}

int FSoundManager::PlaySE(const std::string& path, bool loop) {
  int master = GetMasterHandle(path);
  if (master == -1) return -1;

  int playHandle = DuplicateSoundMem(master);
  if (playHandle == -1) return -1;

  SetVolume(playHandle, 1.0f);

  PlaySoundMem(playHandle, loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK, TRUE);

  PlayHandles.push_back(playHandle);  // 再生用ハンドルを管理リストに登録

  return playHandle;
}

int FSoundManager::PlayBGM(const std::string& path, bool loop) {
  if (BGMHandle != -1) {
    Stop(BGMHandle);
    BGMHandle = -1;
  }

  BGMHandle = PlaySE(path, loop);
  return BGMHandle;
}

void FSoundManager::SetVolume(int handle, float volume) {
  if (handle == -1) return;

  float clampedVolume = std::clamp(volume, 0.0f, 1.0f);
  float clampedMasterVolume = std::clamp(MasterVolume, 0.0f, 1.0f);
  int vol = static_cast<int>(clampedVolume * clampedMasterVolume * 255.0f);
  ChangeVolumeSoundMem(vol, handle);
}

void FSoundManager::Stop(int handle) {
  if (handle == -1) return;

  StopSoundMem(handle);
}
