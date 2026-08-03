#pragma once
#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API FileDialog {
 public:
  static std::string OpenFile(const char* Filter, const std::string& InitialDir = "");
  static std::string SaveFile(
      const char* Filter, const char* DefaultExt = "json", const std::string& InitialDir = ""
  );
  static std::string SelectFolder();
};
