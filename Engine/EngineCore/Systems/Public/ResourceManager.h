#pragma once

#include <string>

#include "BroccoliEngineAPI.h"
#include "FPath.h"
#include "UMath.h"

/// @brief リソースを管理するクラス
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

  /// @brief 画像をロードし、ハンドルを取得する。
  /// @param Path
  /// @return
  int LoadResourceGraph(const FPath& Path);
  /// @brief 3Dモデルをロードし、ハンドルを取得する。
  /// @param Path
  /// @return
  int LoadResourceModel(const FPath& Path);
  bool IsModelValid(int Handle) const;
  bool GetModelBounds(int Handle, FBox3D& OutBounds) const;
  static int NormalizeFontWeight(int Weight);

  /// @brief フォントハンドルを取得する。
  /// @param Size
  /// @param Weight
  /// @return
  int GetFont(int Size, int Weight = DefaultFontWeight);
  int GetTextWidth(const std::string& Text, int FontHandle);
  int GetFontPixelSize(int FontHandle) const;
  void ReleaseResourceGraph();
  void ReleaseAllResources();

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
