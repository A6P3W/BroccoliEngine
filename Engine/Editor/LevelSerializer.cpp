#include "LevelSerializer.h"
#include "ActorRegistry.h"
#include "ObjectManager.h"
#include "Actor.h"
#include "nlohmann/json.hpp"
#include <fstream>

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

    auto& registry = ActorRegistry::GetInstance();
    for (const auto& data : actors)
    {
        AActor* actor = registry.Spawn(data.ClassName, data.Location, data.Rotation);
        if (actor)
        {
            actor->SetActorScale(data.Scale);
        }
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
        obj["location"] = { {"x", d.Location.X}, {"y", d.Location.Y} };
        obj["rotation"] = d.Rotation;
        obj["scale"] = d.Scale;
        arr.push_back(obj);
    }
    root["actors"] = arr;

    std::ofstream ofs(filePath);
    if (!ofs.is_open()) return false;
    ofs << root.dump(4); // 4スペースインデント
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
        data.Location.X = obj["location"].value("x", 0.0f);
        data.Location.Y = obj["location"].value("y", 0.0f);
        data.Rotation = obj.value("rotation", 0.0f);
        data.Scale = obj.value("scale", 1.0f);
        outActors.push_back(data);
    }
    return true;
}
