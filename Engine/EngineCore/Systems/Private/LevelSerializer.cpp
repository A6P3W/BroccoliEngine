#include "LevelSerializer.h"
#include "ActorRegistry.h"
#include "ObjectManager.h"
#include "Actor.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "Log.h"
#include "World.h"
#include "EditorSelectPointComponent.h"
#include "SpriteActor.h"
using json = nlohmann::json;

bool LevelSerializer::Save(World* world, const std::string& filePath)
{
	std::vector<FActorSaveData> actors;
	auto& registry = ActorRegistry::GetInstance();

	for (const auto& actorPtr : world->GetObjectManager()->GetAllActors())
	{
		AActor* actor = actorPtr.get();
		if (!actor || actor->IsPendingDestroy()) continue;
		const std::string name = actor->GetActorClassName();
		if (!registry.Contains(name)) continue;

		FActorSaveData data;
		data.ClassName = name;
		data.Location = actor->GetActorLocation();
		data.Rotation = actor->GetActorRotation();
		data.Scale = actor->GetActorScale();

		// ASpriteActorの場合は画像パスを保存
		if (auto spriteActor = dynamic_cast<ASpriteActor*>(actor))
		{
			data.CustomProperties["ImagePath"] = spriteActor->GetImagePath();
		}

		actors.push_back(data);
	}

	return SaveData(filePath, actors);
}

bool LevelSerializer::Load(World* world, const std::string& filePath)
{
	std::vector<FActorSaveData> actors;
	if (!LoadData(filePath, actors)) return false;

	M_LOG("Loaded actor count: {}", actors.size());

	auto& registry = ActorRegistry::GetInstance();
	std::vector<AActor*> spawnedActors;
	for (const auto& data : actors)
	{
		M_LOG("Spawning: {}", data.ClassName);
		AActor* actor = registry.Spawn(world, data.ClassName, data.Location, data.Rotation);
		if (!actor)
		{
			M_LOG("Spawn failed: {} (not registered?)", data.ClassName);
			continue;
		}
		actor->SetActorScale(data.Scale);

		// ASpriteActorの場合は画像パスを復元
		if (auto spriteActor = dynamic_cast<ASpriteActor*>(actor))
		{
			auto it = data.CustomProperties.find("ImagePath");
			if (it != data.CustomProperties.end())
			{
				spriteActor->SetImagePath(it->second);
			}
		}

		spawnedActors.push_back(actor);
	}
	if (!world->IsSimulating()) {
		return true;
	}
	for (auto* actor : spawnedActors)
	{
		if (actor) actor->Spawned();
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
			{"rotation", d.Rotation.Rotation},
			{"scale",    d.Scale.Scale}
		};

		// プロパティが存在する場合はJSONに出力
		if (!d.CustomProperties.empty()) {
			obj["properties"] = d.CustomProperties;
		}

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
			data.Rotation = FRotator(t.value("rotation", 0.0f));
			data.Scale = FScale(t.value("scale", 1.0f));
		}

		// プロパティが含まれている場合は読み取る
		if (obj.contains("properties")) {
			for (auto& [key, val] : obj["properties"].items()) {
				if (val.is_string()) {
					data.CustomProperties[key] = val.get<std::string>();
				}
			}
		}

		outActors.push_back(data);
	}
	return true;
}
