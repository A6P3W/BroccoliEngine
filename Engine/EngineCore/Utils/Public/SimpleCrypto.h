#pragma once
#include "BroccoliEngineAPI.h"

#include <string>

class BROCCOLI_ENGINE_API SimpleCrypto {
 public:
  static std::string Process(const std::string& input);

 private:
  struct Impl;
  static Impl* GetImpl();
};
