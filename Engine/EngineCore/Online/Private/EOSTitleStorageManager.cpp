#include "EOSTitleStorageManager.h"

#include <eos_titlestorage.h>
#include <minizip/unzip.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>
#include <vector>

#include "DebugOverlay.h"
#include "EOSAuthManager.h"
#include "EOSCoreManager.h"

namespace {
namespace Fs = std::filesystem;

constexpr uint32_t ReadChunkLengthBytes = 64 * 1024;
constexpr size_t ArchiveReadBufferBytes = 64 * 1024;

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
  return !Path.is_absolute() && Path.filename() == Path;
}

bool IsZipFile(const Fs::path& Path) {
  std::string Extension = Path.extension().string();
  std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](unsigned char Character) {
    return static_cast<char>(std::tolower(Character));
  });
  return Extension == ".zip";
}

bool IsSafeArchiveEntry(const std::string& EntryName, const Fs::path& EntryPath) {
  if (EntryName.empty() || EntryName.find('\0') != std::string::npos ||
      EntryName.find(':') != std::string::npos || EntryPath.empty() || EntryPath.is_absolute() ||
      EntryPath.has_root_name()) {
    return false;
  }
  for (const Fs::path& Part : EntryPath) {
    if (Part == "..") {
      return false;
    }
  }
  return true;
}

bool ExtractZipArchive(const Fs::path& ArchivePath, std::string& ErrorMessage) {
  unzFile Archive = unzOpen64(ArchivePath.string().c_str());
  if (!Archive) {
    ErrorMessage = "Failed to open the downloaded ZIP archive.";
    return false;
  }

  const auto CloseArchive = [&Archive]() {
    if (Archive) {
      unzClose(Archive);
      Archive = nullptr;
    }
  };
  const Fs::path ExtractionDirectory = ArchivePath.parent_path();
  int EntryResult = unzGoToFirstFile(Archive);
  if (EntryResult == UNZ_END_OF_LIST_OF_FILE) {
    CloseArchive();
    return true;
  }

  while (EntryResult == UNZ_OK) {
    unz_file_info64 FileInfo = {};
    if (unzGetCurrentFileInfo64(Archive, &FileInfo, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK) {
      ErrorMessage = "Failed to read ZIP entry information.";
      CloseArchive();
      return false;
    }

    std::vector<char> EntryNameBuffer(static_cast<size_t>(FileInfo.size_filename) + 1, '\0');
    if (unzGetCurrentFileInfo64(
            Archive,
            &FileInfo,
            EntryNameBuffer.data(),
            static_cast<uLong>(EntryNameBuffer.size()),
            nullptr,
            0,
            nullptr,
            0
        ) != UNZ_OK) {
      ErrorMessage = "Failed to read a ZIP entry name.";
      CloseArchive();
      return false;
    }

    const std::string EntryName(EntryNameBuffer.data(), static_cast<size_t>(FileInfo.size_filename));
    const Fs::path EntryPath(EntryName);
    if (!IsSafeArchiveEntry(EntryName, EntryPath)) {
      ErrorMessage = "The ZIP archive contains an unsafe entry path.";
      CloseArchive();
      return false;
    }

    const Fs::path OutputPath = ExtractionDirectory / EntryPath;
    const bool IsDirectory = EntryName.back() == '/' || EntryName.back() == '\\';
    std::error_code ErrorCode;
    if (IsDirectory) {
      Fs::create_directories(OutputPath, ErrorCode);
      if (ErrorCode) {
        ErrorMessage = "Failed to create a directory while extracting the ZIP archive.";
        CloseArchive();
        return false;
      }
    } else {
      Fs::create_directories(OutputPath.parent_path(), ErrorCode);
      if (ErrorCode) {
        ErrorMessage = "Failed to create a directory while extracting the ZIP archive.";
        CloseArchive();
        return false;
      }

      if (unzOpenCurrentFile(Archive) != UNZ_OK) {
        ErrorMessage = "Failed to open a file in the ZIP archive.";
        CloseArchive();
        return false;
      }

      const Fs::path TemporaryOutputPath = OutputPath.string() + ".extracting";
      Fs::remove(TemporaryOutputPath, ErrorCode);
      ErrorCode.clear();
      std::ofstream Output(TemporaryOutputPath, std::ios::binary | std::ios::trunc);
      if (!Output.is_open()) {
        unzCloseCurrentFile(Archive);
        ErrorMessage = "Failed to create a file while extracting the ZIP archive.";
        CloseArchive();
        return false;
      }

      std::array<char, ArchiveReadBufferBytes> Buffer = {};
      int ReadResult = 0;
      while ((ReadResult = unzReadCurrentFile(
                  Archive, Buffer.data(), static_cast<unsigned int>(Buffer.size())
              )) > 0) {
        Output.write(Buffer.data(), ReadResult);
        if (!Output.good()) {
          break;
        }
      }
      Output.close();
      const int CloseResult = unzCloseCurrentFile(Archive);
      if (ReadResult < 0 || !Output.good() || CloseResult != UNZ_OK) {
        Fs::remove(TemporaryOutputPath, ErrorCode);
        ErrorMessage = "Failed to extract a file from the ZIP archive.";
        CloseArchive();
        return false;
      }

      Fs::remove(OutputPath, ErrorCode);
      ErrorCode.clear();
      Fs::rename(TemporaryOutputPath, OutputPath, ErrorCode);
      if (ErrorCode) {
        Fs::remove(TemporaryOutputPath, ErrorCode);
        ErrorMessage = "Failed to finalize an extracted ZIP file.";
        CloseArchive();
        return false;
      }
    }

    EntryResult = unzGoToNextFile(Archive);
  }

  CloseArchive();
  if (EntryResult != UNZ_END_OF_LIST_OF_FILE) {
    ErrorMessage = "Failed while enumerating ZIP archive entries.";
    return false;
  }
  return true;
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
    std::string CompletedLocalPath = CurrentLocalPath;
    std::string CompletionMessage = "File download completed.";
    if (IsZipFile(CurrentFinalPath)) {
      std::string ExtractionError;
      if (!ExtractZipArchive(CurrentFinalPath, ExtractionError)) {
        DRAW_SCREEN_LOG(
            "EOSTitleStorage",
            8.0f,
            "[EOSTitleStorage] ZIP extraction failed: FileName={} Message={}",
            CompletedFileName,
            ExtractionError
        );
        Complete(
            MakeResult(
                false,
                ETitleStorageDownloadError::ArchiveExtractionFailed,
                ExtractionError,
                CompletedFileName,
                CompletedLocalPath
            ),
            false
        );
        return;
      }

      Fs::remove(CurrentFinalPath, ErrorCode);
      if (ErrorCode) {
        Complete(
            MakeResult(
                false,
                ETitleStorageDownloadError::ArchiveDeleteFailed,
                "ZIP extraction completed, but the ZIP file could not be deleted.",
                CompletedFileName,
                CompletedLocalPath
            ),
            false
        );
        return;
      }
      CompletedLocalPath = CurrentFinalPath.parent_path().generic_string();
      CompletionMessage = "ZIP download and extraction completed.";
    }

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
            CompletionMessage,
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

std::string EOSTitleStorageManager::GetDownloadDirectoryPath() {
  return Fs::path("Resources-EOS").generic_string();
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
    return Reject(ETitleStorageDownloadError::InvalidFileName, "Enter a file name without a path.");
  }

  EOS_HTitleStorage TitleStorageHandle = EOSCoreManager::GetInstance().GetTitleStorageHandle();
  if (!TitleStorageHandle) {
    return Reject(
        ETitleStorageDownloadError::EOSNotInitialized,
        "The EOS Title Storage interface is unavailable."
    );
  }

  const Fs::path DownloadDirectory = GetDownloadDirectoryPath();
  std::error_code ErrorCode;
  Fs::create_directories(DownloadDirectory, ErrorCode);
  if (ErrorCode || !Fs::is_directory(DownloadDirectory, ErrorCode)) {
    return Reject(
        ETitleStorageDownloadError::DirectoryCreationFailed, "Failed to create Resources-EOS."
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
