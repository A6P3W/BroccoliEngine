#pragma once
#include "BroccoliEngineAPI.h"
#include <string>

class BROCCOLI_ENGINE_API FileDialog {
 public:
  static std::string OpenFile(const char* filter);
  static std::string SaveFile(const char* filter, const char* defaultExt = "json");
};
