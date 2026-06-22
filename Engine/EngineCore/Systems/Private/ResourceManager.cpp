#include "ResourceManager.h"
#include <DxLib.h>
#include <string>

ResourceManager::ResourceManager()
{
    DefaultGraph = LoadResourceGraph("Engine/EngineSide/Files/texture_Checker_64px.png");
}

ResourceManager& ResourceManager::GetInstance()
{
    static ResourceManager instance;
    return instance;
}


int ResourceManager::LoadResourceGraph(const std::string& path)
{
    auto it = GraphMap.find(path);
    if (it != GraphMap.end()) {
        return it->second;
    }
    int handle = LoadGraph(path.c_str());
    if (handle == -1) {
        handle = DefaultGraph;
    }
    GraphMap[path] = handle;
    return handle;
}

int ResourceManager::GetFont(int size, int thickness)
{
    std::string key = std::to_string(size) + "_" + std::to_string(thickness);
    if(FontMap.count(key)) return FontMap[key];

    int handle = CreateFontToHandle(NULL, size, thickness);
    FontMap[key] = handle;
    return handle;
}

void ResourceManager::ReleaseResourceGraph()
{
    for (auto& pair : GraphMap) DeleteGraph(pair.second);
    for (auto& pair : FontMap) DeleteFontToHandle(pair.second);
}
