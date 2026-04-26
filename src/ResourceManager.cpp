#include "ResourceManager.h"
#include <DxLib.h>
#include <string>


int ResourceManager::LoadResourceGraph(const std::string& path)
{
    auto it = graphMap.find(path);
    if (it != graphMap.end()) {
        return it->second;
    }
    int handle = LoadGraph(path.c_str());
    graphMap[path] = handle;
    return handle;
}
