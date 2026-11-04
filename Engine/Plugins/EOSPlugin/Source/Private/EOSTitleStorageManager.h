#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "BroccoliEngineAPI.h"
#include "EOSTypes.h"
class EOSTitleStorageManager {
 public:
  using CompletionCallback = std::function<void(const FTitleStorageDownloadResult&)>;

  static EOSTitleStorageManager& GetInstance();
  static std::string GetDownloadDirectoryPath();

  bool Download(const std::string& FileName, CompletionCallback OnComplete);
  bool IsDownloadPending() const;
  void Shutdown();

 private:
  EOSTitleStorageManager();
  ~EOSTitleStorageManager();
  EOSTitleStorageManager(const EOSTitleStorageManager&) = delete;
  EOSTitleStorageManager& operator=(const EOSTitleStorageManager&) = delete;

  struct Impl;
  Impl* ImplPtr = nullptr;
};
