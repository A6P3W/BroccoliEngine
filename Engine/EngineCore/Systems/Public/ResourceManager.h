#pragma once

#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API ResourceManager {
 public:
  ResourceManager();
  ~ResourceManager();
  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;

  static ResourceManager& GetInstance();

  int LoadResourceGraph(const std::string& Path);
  int GetFont(int Size, int Thickness);
  int GetTextWidth(const std::string& Text, int FontHandle);
  int GetFontPixelSize(int FontHandle) const;
  void ReleaseResourceGraph();

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
