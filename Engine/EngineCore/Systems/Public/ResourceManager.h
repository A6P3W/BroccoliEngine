#pragma once
#include <map>
#include <string>
class ResourceManager
{
public:
	ResourceManager();
	static ResourceManager& GetInstance();
	int LoadResourceGraph(const std::string& path);
	int GetFont(int size, int thickness);
	void ReleaseResourceGraph();
private:
	std::map<std::string, int> GraphMap;
	std::map<std::string, int> FontMap;
	int DefaultGraph;
};

