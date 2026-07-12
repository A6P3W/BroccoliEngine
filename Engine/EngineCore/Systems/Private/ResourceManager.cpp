#include "ResourceManager.h"

#include <DxLib.h>

#include <string>

struct ResourceManager::Impl {
  std::map<std::string, int> GraphMap;
  std::map<std::string, int> FontMap;
  int DefaultGraph = -1;
};

ResourceManager::ResourceManager() : ImplPtr(new Impl()) {
  ImplPtr->DefaultGraph = LoadResourceGraph("Engine/EngineSide/Files/texture_Checker_64px.png");
}

ResourceManager::~ResourceManager() {
  delete ImplPtr;
}

ResourceManager& ResourceManager::GetInstance() {
  static ResourceManager instance;
  return instance;
}

int ResourceManager::LoadResourceGraph(const std::string& path) {
  auto it = ImplPtr->GraphMap.find(path);
  if (it != ImplPtr->GraphMap.end()) {
    return it->second;
  }
  int handle = LoadGraph(path.c_str());
  if (handle == -1) {
    handle = ImplPtr->DefaultGraph;
  }
  ImplPtr->GraphMap[path] = handle;
  return handle;
}

int ResourceManager::GetFont(int size, int thickness) {
  std::string key = std::to_string(size) + "_" + std::to_string(thickness);
  if (ImplPtr->FontMap.count(key)) return ImplPtr->FontMap[key];

  int handle = CreateFontToHandle(NULL, size, thickness);
  ImplPtr->FontMap[key] = handle;
  return handle;
}

void ResourceManager::ReleaseResourceGraph() {
  for (auto& pair : ImplPtr->GraphMap) DeleteGraph(pair.second);
  for (auto& pair : ImplPtr->FontMap) DeleteFontToHandle(pair.second);
}
