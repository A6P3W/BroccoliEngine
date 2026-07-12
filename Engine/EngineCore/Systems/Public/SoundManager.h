#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API FSoundManager {
 public:
  FSoundManager() = default;
  ~FSoundManager();

  FSoundManager(const FSoundManager&) = delete;
  FSoundManager& operator=(const FSoundManager&) = delete;

  // マスター音源をロード（内部用）
  int GetMasterHandle(const std::string& path);

  // 再生して「操作用ハンドル」を返す
  int PlaySE(const std::string& path, bool loop = false);
  int PlayBGM(const std::string& path, bool loop = true);

  // ハンドルに対して操作
  void SetVolume(int handle, float volume);  // volume: 0.0f ~ 1.0f
  void Stop(int handle);

  // 全体の音量
  void SetMasterVolume(float vol) { MasterVolume = vol; }

 private:
  std::unordered_map<std::string, int> MasterSounds;
  std::vector<int> PlayHandles;  // 複製した再生用ハンドルの管理リスト
  int BGMHandle = -1;
  float MasterVolume = 1.0f;
};
