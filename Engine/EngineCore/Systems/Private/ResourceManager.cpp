#include "ResourceManager.h"
#include <DxLib.h>
#include <string>

ResourceManager::ResourceManager()
{
    default_graph = LoadResourceGraph("Engine/EngineSide/Files/texture_Checker_64px.png");
}

ResourceManager& ResourceManager::GetInstance()
{
    static ResourceManager instance;
    return instance;
}


int ResourceManager::LoadResourceGraph(const std::string& path)
{
    auto it = graphMap.find(path);
    if (it != graphMap.end()) {
        return it->second;
    }
    int handle = LoadGraph(path.c_str());
    if (handle == -1) {
        handle = default_graph;
    }
    graphMap[path] = handle;
    return handle;
}

int ResourceManager::GetFont(int size, int thickness)
{
    std::string key = std::to_string(size) + "_" + std::to_string(thickness);
    if(fontMap.count(key)) return fontMap[key];

    int handle = CreateFontToHandle(NULL, size, thickness);
    fontMap[key] = handle;
    return handle;
}

void ResourceManager::ReleaseResourceGraph()
{
    for (auto& pair : graphMap) DeleteGraph(pair.second);
    for (auto& pair : fontMap) DeleteFontToHandle(pair.second);
}
