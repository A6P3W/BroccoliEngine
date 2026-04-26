#pragma once
#include <map>
#include <string>
class ResourceManager
{
public:
	static ResourceManager& GetInstance() {
		static ResourceManager instance;
		return instance;
	}
	int LoadResourceGraph(const std::string& path);

private:
	std::map<std::string, int> graphMap;
};

