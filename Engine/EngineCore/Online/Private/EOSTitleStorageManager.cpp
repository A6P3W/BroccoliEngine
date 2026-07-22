#include "EOSTitleStorageManager.h"

#include <eos_titlestorage.h>

#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

#include "DebugOverlay.h"
#include "EOSAuthManager.h"
#include "EOSCoreManager.h"

namespace {
namespace Fs = std::filesystem;

constexpr uint32_t ReadChunkLengthBytes = 64 * 1024;

const char* SafeEOSResult(EOS_EResult Result) {
  const char* Text = EOS_EResult_ToString(Result);
  return Text ? Text : "<null>";
}

bool IsValidFileName(const std::string& FileName) {
  if (FileName.empty() || FileName.find('/') != std::string::npos ||
      FileName.find('\\') != std::string::npos || FileName.find("..") != std::string::npos ||
      FileName.find_first_of("<>:\"|?*") != std::string::npos) {
    return false;
  }

  for (const unsigned char Character : FileName) {
    if (Character < 32) {
      return false;
    }
  }

  const Fs::path Path(FileName);
  return !Path.is_absolute() && Path.filename() == Path && Path.extension() == ".BLevel";
}

FTitleStorageDownloadResult MakeResult(
    bool Success,
    ETitleStorageDownloadError Error,
    std::string Message,
    const std::string& FileName,
    std::string LocalFilePath = {}
) {
  FTitleStorageDownloadResult Result;
  Result.Success = Success;
  Result.Error = Error;
  Result.Message = std::move(Message);
  Result.FileName = FileName;
  Result.LocalFilePath = std::move(LocalFilePath);
  return Result;
}
}  // namespace

struct EOSTitleStorageManager::Impl {
  bool DownloadPending = false;
  bool WriteFailed = false;
  EOS_HTitleStorageFileTransferRequest TransferRequestHandle = nullptr;
  std::string CurrentFileName;
  std::string CurrentLocalPath;
  Fs::path CurrentFinalPath;
  Fs::path CurrentTemporaryPath;
  std::ofstream OutputStream;
  CompletionCallback OnComplete;

  void ReleaseTransferRequest() {
    if (TransferRequestHandle) {
      EOS_TitleStorageFileTransferRequest_Release(TransferRequestHandle);
      TransferRequestHandle = nullptr;
    }
  }

  void Complete(const FTitleStorageDownloadResult& Result, bool RemoveTemporaryFile) {
    if (!DownloadPending) {
      return;
    }

    if (OutputStream.is_open()) {
      OutputStream.close();
    }
    ReleaseTransferRequest();

    if (RemoveTemporaryFile && !CurrentTemporaryPath.empty()) {
      std::error_code ErrorCode;
      Fs::remove(CurrentTemporaryPath, ErrorCode);
    }

    DownloadPending = false;
    WriteFailed = false;
    CurrentFileName.clear();
    CurrentLocalPath.clear();
    CurrentFinalPath.clear();
    CurrentTemporaryPath.clear();
    CompletionCallback Callback = std::move(OnComplete);
    OnComplete = nullptr;
    if (Callback) {
      Callback(Result);
    }
  }

  void FinishRead(EOS_EResult EOSResult) {
    if (!DownloadPending) {
      return;
    }

    if (OutputStream.is_open()) {
      OutputStream.close();
    }

    if (WriteFailed) {
      DRAW_SCREEN_LOG(
          "EOSTitleStorage",
          8.0f,
          "[EOSTitleStorage] Download failed: file write failed. FileName={}",
          CurrentFileName
      );
      Complete(
          MakeResult(
              false,
              ETitleStorageDownloadError::FileWriteFailed,
              "Failed to write the downloaded level.",
              CurrentFileName
          ),
          true
      );
      return;
    }

    if (EOSResult != EOS_EResult::EOS_Success) {
      DRAW_SCREEN_LOG(
          "EOSTitleStorage",
          8.0f,
          "[EOSTitleStorage] Download failed: FileName={} Result={}",
          CurrentFileName,
          SafeEOSResult(EOSResult)
      );
      Complete(
          MakeResult(
              false,
              ETitleStorageDownloadError::DownloadFailed,
              std::string("EOS download failed: ") + SafeEOSResult(EOSResult),
              CurrentFileName
          ),
          true
      );
      return;
    }

    std::error_code ErrorCode;
    if (!Fs::exists(CurrentTemporaryPath, ErrorCode) || ErrorCode) {
      DRAW_SCREEN_LOG(
          "EOSTitleStorage",
          8.0f,
          "[EOSTitleStorage] Download failed: temporary file is missing. FileName={}",
          CurrentFileName
      );
      Complete(
          MakeResult(
              false,
              ETitleStorageDownloadError::FileRenameFailed,
              "The downloaded temporary file is missing.",
              CurrentFileName
          ),
          true
      );
      return;
    }

    const Fs::path BackupPath = CurrentFinalPath.string() + ".backup";
    const bool HadFinalFile = Fs::exists(CurrentFinalPath, ErrorCode) && !ErrorCode;
    if (HadFinalFile) {
      Fs::remove(BackupPath, ErrorCode);
      ErrorCode.clear();
      Fs::rename(CurrentFinalPath, BackupPath, ErrorCode);
      if (ErrorCode) {
        DRAW_SCREEN_LOG(
            "EOSTitleStorage",
            8.0f,
            "[EOSTitleStorage] Existing file backup failed: {}",
            ErrorCode.message()
        );
        Complete(
            MakeResult(
                false,
                ETitleStorageDownloadError::FinalFileDeleteFailed,
                "Failed to prepare the existing level for replacement.",
                CurrentFileName
            ),
            true
        );
        return;
      }
    }

    ErrorCode.clear();
    Fs::rename(CurrentTemporaryPath, CurrentFinalPath, ErrorCode);
    if (ErrorCode) {
      if (HadFinalFile) {
        std::error_code RestoreError;
        Fs::rename(BackupPath, CurrentFinalPath, RestoreError);
      }
      DRAW_SCREEN_LOG(
          "EOSTitleStorage",
          8.0f,
          "[EOSTitleStorage] Final file rename failed: {}",
          ErrorCode.message()
      );
      Complete(
          MakeResult(
              false,
              ETitleStorageDownloadError::FileRenameFailed,
              "Failed to finalize the downloaded level.",
              CurrentFileName
          ),
          true
      );
      return;
    }

    if (HadFinalFile) {
      Fs::remove(BackupPath, ErrorCode);
    }
    ErrorCode.clear();
    if (!Fs::exists(CurrentFinalPath, ErrorCode) || ErrorCode) {
      DRAW_SCREEN_LOG(
          "EOSTitleStorage",
          8.0f,
          "[EOSTitleStorage] Final file verification failed. FileName={}",
          CurrentFileName
      );
      Complete(
          MakeResult(
              false,
              ETitleStorageDownloadError::FileRenameFailed,
              "The finalized downloaded level could not be verified.",
              CurrentFileName
          ),
          true
      );
      return;
    }

    const std::string CompletedFileName = CurrentFileName;
    const std::string CompletedLocalPath = CurrentLocalPath;
    DRAW_SCREEN_LOG(
        "EOSTitleStorage",
        8.0f,
        "[EOSTitleStorage] Download succeeded: FileName={} LocalPath={}",
        CompletedFileName,
        CompletedLocalPath
    );
    Complete(
        MakeResult(
            true,
            ETitleStorageDownloadError::None,
            "Level download completed.",
            CompletedFileName,
            CompletedLocalPath
        ),
        false
    );
  }
};

EOSTitleStorageManager& EOSTitleStorageManager::GetInstance() {
  static EOSTitleStorageManager Instance;
  return Instance;
}

EOSTitleStorageManager::EOSTitleStorageManager() : ImplPtr(new Impl()) {}

EOSTitleStorageManager::~EOSTitleStorageManager() { delete ImplPtr; }

bool EOSTitleStorageManager::Download(const std::string& FileName, CompletionCallback OnComplete) {
  DRAW_SCREEN_LOG(
      "EOSTitleStorage", 8.0f, "[EOSTitleStorage] Download requested: FileName={}", FileName
  );

  const auto Reject = [&FileName,
                       &OnComplete](ETitleStorageDownloadError Error, const std::string& Message) {
    DRAW_SCREEN_LOG(
        "EOSTitleStorage",
        8.0f,
        "[EOSTitleStorage] Download rejected: FileName={} Message={}",
        FileName,
        Message
    );
    if (OnComplete) {
      OnComplete(MakeResult(false, Error, Message, FileName));
    }
    return false;
  };

  if (ImplPtr->DownloadPending) {
    return Reject(
        ETitleStorageDownloadError::DownloadInProgress,
        "Another title storage download is already in progress."
    );
  }
  if (!EOSCoreManager::GetInstance().IsInitialized()) {
    return Reject(ETitleStorageDownloadError::EOSNotInitialized, "EOS is not initialized.");
  }
  if (!EOSAuthManager::GetInstance().IsLoggedIn()) {
    return Reject(ETitleStorageDownloadError::NotLoggedIn, "EOS login is required.");
  }
  if (!IsValidFileName(FileName)) {
    return Reject(
        ETitleStorageDownloadError::InvalidFileName, "Enter a .BLevel file name without a path."
    );
  }

  EOS_HTitleStorage TitleStorageHandle = EOSCoreManager::GetInstance().GetTitleStorageHandle();
  if (!TitleStorageHandle) {
    return Reject(
        ETitleStorageDownloadError::EOSNotInitialized,
        "The EOS Title Storage interface is unavailable."
    );
  }

  const Fs::path DownloadDirectory = Fs::path("Saved") / "Downloaded";
  std::error_code ErrorCode;
  Fs::create_directories(DownloadDirectory, ErrorCode);
  if (ErrorCode || !Fs::is_directory(DownloadDirectory, ErrorCode)) {
    return Reject(
        ETitleStorageDownloadError::DirectoryCreationFailed, "Failed to create Saved/Downloaded."
    );
  }

  const Fs::path FinalPath = DownloadDirectory / Fs::path(FileName);
  const Fs::path TemporaryPath = FinalPath.string() + ".download";
  Fs::remove(TemporaryPath, ErrorCode);
  ErrorCode.clear();

  ImplPtr->OutputStream.open(TemporaryPath, std::ios::binary | std::ios::trunc);
  if (!ImplPtr->OutputStream.is_open()) {
    return Reject(
        ETitleStorageDownloadError::TemporaryFileOpenFailed,
        "Failed to create the temporary download file."
    );
  }

  ImplPtr->DownloadPending = true;
  ImplPtr->WriteFailed = false;
  ImplPtr->CurrentFileName = FileName;
  ImplPtr->CurrentLocalPath = FinalPath.generic_string();
  ImplPtr->CurrentFinalPath = FinalPath;
  ImplPtr->CurrentTemporaryPath = TemporaryPath;
  ImplPtr->OnComplete = std::move(OnComplete);

  DRAW_SCREEN_LOG(
      "EOSTitleStorage",
      8.0f,
      "[EOSTitleStorage] Download starting: FileName={} TemporaryPath={} LocalPath={}",
      FileName,
      TemporaryPath.generic_string(),
      ImplPtr->CurrentLocalPath
  );

  EOS_TitleStorage_ReadFileOptions Options = {};
  Options.ApiVersion = EOS_TITLESTORAGE_READFILE_API_LATEST;
  Options.LocalUserId = EOSAuthManager::GetInstance().GetLocalUserId();
  Options.Filename = ImplPtr->CurrentFileName.c_str();
  Options.ReadChunkLengthBytes = ReadChunkLengthBytes;
  Options.ReadFileDataCallback = [](const EOS_TitleStorage_ReadFileDataCallbackInfo* Data) {
    auto* State = static_cast<Impl*>(Data ? Data->ClientData : nullptr);
    if (!State || !State->DownloadPending || State->WriteFailed || !State->OutputStream.is_open()) {
      return EOS_TitleStorage_EReadResult::EOS_TS_RR_FailRequest;
    }
    if (Data->DataChunkLengthBytes > 0 && Data->DataChunk) {
      State->OutputStream.write(
          static_cast<const char*>(Data->DataChunk), Data->DataChunkLengthBytes
      );
      if (!State->OutputStream.good()) {
        State->WriteFailed = true;
        DRAW_SCREEN_LOG(
            "EOSTitleStorage",
            8.0f,
            "[EOSTitleStorage] File write failed: FileName={}",
            State->CurrentFileName
        );
        return EOS_TitleStorage_EReadResult::EOS_TS_RR_FailRequest;
      }
    }
    return EOS_TitleStorage_EReadResult::EOS_TS_RR_ContinueReading;
  };
  Options.FileTransferProgressCallback = nullptr;

  ImplPtr->TransferRequestHandle = EOS_TitleStorage_ReadFile(
      TitleStorageHandle, &Options, ImplPtr, [](const EOS_TitleStorage_ReadFileCallbackInfo* Data) {
        auto* State = static_cast<Impl*>(Data ? Data->ClientData : nullptr);
        if (State) {
          State->FinishRead(Data ? Data->ResultCode : EOS_EResult::EOS_UnexpectedError);
        }
      }
  );

  if (!ImplPtr->TransferRequestHandle) {
    const FTitleStorageDownloadResult Result = MakeResult(
        false,
        ETitleStorageDownloadError::DownloadStartFailed,
        "EOS failed to start the title storage download.",
        FileName
    );
    DRAW_SCREEN_LOG(
        "EOSTitleStorage", 8.0f, "[EOSTitleStorage] Download start failed: FileName={}", FileName
    );
    ImplPtr->Complete(Result, true);
    return false;
  }

  return true;
}

bool EOSTitleStorageManager::IsDownloadPending() const { return ImplPtr->DownloadPending; }

void EOSTitleStorageManager::Shutdown() {
  if (!ImplPtr->DownloadPending) {
    return;
  }

  DRAW_SCREEN_LOG(
      "EOSTitleStorage", 8.0f, "[EOSTitleStorage] Canceling active download during shutdown."
  );
  if (ImplPtr->TransferRequestHandle) {
    EOS_TitleStorageFileTransferRequest_CancelRequest(ImplPtr->TransferRequestHandle);
  }
  ImplPtr->Complete(
      MakeResult(
          false,
          ETitleStorageDownloadError::DownloadFailed,
          "Title storage download canceled during shutdown.",
          ImplPtr->CurrentFileName
      ),
      true
  );
}
