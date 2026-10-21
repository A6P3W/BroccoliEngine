#pragma once
#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API SimpleCrypto {
 public:
  /// @brief 文字列を暗号化する。BLevel.json->BLevel用。
  /// @param input
  /// @return
  static std::string Process(const std::string& input);

 private:
  struct Impl;
  static Impl* GetImpl();
};
