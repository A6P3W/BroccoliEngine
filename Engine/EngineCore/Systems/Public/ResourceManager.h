#pragma once

#include <string>

#include "BroccoliEngineAPI.h"
#include "FPath.h"
#include "Material3D.h"
#include "UMath.h"

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

  int LoadResourceGraph(const FPath& Path);
  int LoadResourceModel(const FPath& Path);
  bool IsModelValid(int Handle) const;
  bool GetModelBounds(int Handle, FBox3D& OutBounds) const;
  int CreateMaterial3D(const FMaterial3DDesc& Desc);
  const FMaterial3DDesc* GetMaterial3D(int Handle) const;
  int GetDefaultLitMaterial3D() const;
  int GetDefaultUnlitMaterial3D() const;
  static int NormalizeFontWeight(int Weight);

  int GetFont(int Size, int Weight = DefaultFontWeight);
  int GetTextWidth(const std::string& Text, int FontHandle);
  int GetFontPixelSize(int FontHandle) const;
  void ReleaseResourceGraph();
  void ReleaseAllResources();

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
