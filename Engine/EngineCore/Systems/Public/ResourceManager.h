#pragma once

#include <string>

#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API ResourceManager {
 public:
  static constexpr int MinFontWeight = 100;
  static constexpr int MaxFontWeight = 900;
  static constexpr int FontWeightStep = 100;
  static constexpr int DefaultFontWeight = 400;

  ResourceManager();
  ~ResourceManager();
  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;

  static ResourceManager& GetInstance();

  int LoadResourceGraph(const std::string& Path);
  static int NormalizeFontWeight(int Weight);

  int GetFont(int Size, int Weight = DefaultFontWeight);
  int GetTextWidth(const std::string& Text, int FontHandle);
  int GetFontPixelSize(int FontHandle) const;
  void ReleaseResourceGraph();

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
