#pragma once
#include "BroccoliEngineAPI.h"

#include <string>

class BROCCOLI_ENGINE_API NetworkUtils {
 public:
  static std::string GetLocalIPAddress();
};