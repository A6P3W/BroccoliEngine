#pragma once
#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API FileDialog {
 public:
  static std::string OpenFile(const char* filter);
  static std::string SaveFile(const char* filter, const char* defaultExt = "json");
  static std::string SelectFolder();
};
