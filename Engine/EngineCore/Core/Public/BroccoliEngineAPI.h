#pragma once

#if defined(_WIN32)
  #if defined(BROCCOLI_ENGINE_BUILD)
    #define BROCCOLI_ENGINE_API __declspec(dllexport)
  #else
    #define BROCCOLI_ENGINE_API __declspec(dllimport)
  #endif
#else
  #define BROCCOLI_ENGINE_API
#endif