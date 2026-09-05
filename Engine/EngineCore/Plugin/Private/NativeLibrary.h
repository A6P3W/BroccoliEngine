#pragma once

#include <filesystem>
#include <string>

class NativeLibrary {
 public:
  NativeLibrary() = default;
  ~NativeLibrary();

  NativeLibrary(const NativeLibrary&) = delete;
  NativeLibrary& operator=(const NativeLibrary&) = delete;
  NativeLibrary(NativeLibrary&& Other) noexcept;
  NativeLibrary& operator=(NativeLibrary&& Other) noexcept;

  bool Load(const std::filesystem::path& Path);
  void* GetSymbol(const char* Name) const;
  void Unload();
  bool IsLoaded() const;
  const std::string& GetLastError() const;

 private:
  void* Handle = nullptr;
  mutable std::string LastError;
};
