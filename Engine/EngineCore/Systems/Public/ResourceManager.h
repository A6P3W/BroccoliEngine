#pragma once
#include "BroccoliEngineAPI.h"
#include <map>
#include <string>
class BROCCOLI_ENGINE_API ResourceManager {
 public:
  ResourceManager();
  ~ResourceManager();
  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;
  static ResourceManager& GetInstance();
  int LoadResourceGraph(const std::string& path);
  int GetFont(int size, int thickness);
  void ReleaseResourceGraph();

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
