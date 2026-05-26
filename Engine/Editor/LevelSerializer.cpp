#include "LevelSerializer.h"
#include "ActorRegistry.h"
#include "ObjectManager.h"
#include "Actor.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "Utils/Log.h"
using json = nlohmann::json;

bool LevelSerializer::Save(const std::string& filePath)
{
	std::vector<FActorSaveData> actors;
	auto& registry = ActorRegistry::GetInstance();

	for (const auto& actorPtr : ObjectManager::GetInstance().GetAllActors())
	{
		AActor* actor = actorPtr.get();
		if (!actor || actor->IsPendingDestroy()) continue;
		const std::string name = actor->GetActorClassName();
		if (!registry.Contains(name)) continue;

		FActorSaveData data;
		data.ClassName = name;
		data.Location = actor->GetActorLocation();
		data.Rotation = actor->GetActorRotation().Rotation;
		data.Scale = actor->GetActorScale().Scale;
		actors.push_back(data);
	}

	return SaveData(filePath, actors);
}

bool LevelSerializer::Load(const std::string& filePath)
{
	std::vector<FActorSaveData> actors;
	if (!LoadData(filePath, actors)) return false;

	// ここでactorsが空でないか確認
	M_LOG("Loaded actor count: {}", actors.size());

	auto& registry = ActorRegistry::GetInstance();
	for (const auto& data : actors)
	{
		M_LOG("Spawning: {}", data.ClassName);
		AActor* actor = registry.Spawn(data.ClassName, data.Location, data.Rotation);
		if (!actor)
		{
			M_LOG("Spawn failed: {} (not registered?)", data.ClassName);
			continue;
		}
		actor->SetActorScale(data.Scale);
	}
	return true;
}

bool LevelSerializer::SaveData(const std::string& filePath,
	const std::vector<FActorSaveData>& actors)
{
	json root;
	json arr = json::array();
	for (const auto& d : actors)
	{
		json obj;
		obj["class"] = d.ClassName;
		obj["instance_name"] = d.InstanceName;
		obj["transform"] = {
			{"location", {{"x", d.Location.X}, {"y", d.Location.Y}}},
			{"rotation", d.Rotation},
			{"scale",    d.Scale}
		};
		arr.push_back(obj);
	}
	root["actors"] = arr;
	std::ofstream ofs(filePath);
	if (!ofs.is_open()) return false;
	ofs << root.dump(4);
	return true;
}

bool LevelSerializer::LoadData(const std::string& filePath,
	std::vector<FActorSaveData>& outActors)
{
	std::ifstream ifs(filePath);
	if (!ifs.is_open()) return false;
	json root;
	try { ifs >> root; }
	catch (...) { return false; }
	if (!root.contains("actors")) return false;
	for (const auto& obj : root["actors"])
	{
		FActorSaveData data;
		data.ClassName = obj.value("class", "");
		data.InstanceName = obj.value("instance_name", "");
		// transformネスト対応
		if (obj.contains("transform"))
		{
			const auto& t = obj["transform"];
			if (t.contains("location"))
			{
				data.Location.X = t["location"].value("x", 0.0f);
				data.Location.Y = t["location"].value("y", 0.0f);
			}
			data.Rotation = t.value("rotation", 0.0f);
			data.Scale = t.value("scale", 1.0f);
		}
		outActors.push_back(data);
	}
	return true;
}
