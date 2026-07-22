#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "BroccoliEngineAPI.h"

enum class ETitleStorageDownloadError : uint8_t {
  None,
  EOSNotInitialized,
  NotLoggedIn,
  DownloadInProgress,
  InvalidFileName,
  DirectoryCreationFailed,
  TemporaryFileOpenFailed,
  DownloadStartFailed,
  DownloadFailed,
  FileWriteFailed,
  FinalFileDeleteFailed,
  FileRenameFailed,
  ArchiveExtractionFailed,
  ArchiveDeleteFailed
};

struct FTitleStorageDownloadResult {
  bool Success = false;
  ETitleStorageDownloadError Error = ETitleStorageDownloadError::DownloadFailed;
  std::string Message;
  std::string FileName;
  std::string LocalFilePath;
};

class BROCCOLI_ENGINE_API EOSTitleStorageManager {
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
