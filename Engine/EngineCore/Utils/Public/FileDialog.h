#pragma once
#include <string>

#include "BroccoliEngineAPI.h"

/// @brief windowsファイルダイアログを管理するクラス。
class BROCCOLI_ENGINE_API FileDialog {
 public:
  /**
  * @brief ファイルを開く。
  * 
  * @param Filter 
  * @param InitialDir 
  * @return std::string 
  */
  static std::string OpenFile(const char* Filter, const std::string& InitialDir = "");
  /**
   * @brief ファイルを保存する。
   * 
   * @param Filter 
   * @param DefaultExt 
   * @param InitialDir 
   * @return std::string 
   */
  static std::string SaveFile(
      const char* Filter, const char* DefaultExt = "json", const std::string& InitialDir = ""
  );
  static std::string SelectFolder();
};
