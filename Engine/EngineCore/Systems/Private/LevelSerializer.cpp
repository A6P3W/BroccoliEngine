#include "LevelSerializer.h"

#include <algorithm>
#include <fstream>

#include "Actor.h"
#include "ActorRegistry.h"
#include "EditorSelectPointComponent.h"
#include "GameModeBase.h"
#include "Log.h"
#include "nlohmann/json.hpp"
#include "ActorManager.h"
#include "SimpleCrypto.h"
#include "SpriteActor.h"
#include "World.h"

using json = nlohmann::json;

bool LevelSerializer::Save(
    World* world, const std::string& filePath, const std::string& gameModeClassName
) {
  if (!world) {
    return false;
  }
  std::vector<FActorSaveData> actors;
  auto& registry = ActorRegistry::GetInstance();
  AActor* gameModeActor = world->GetGameMode();
  for (const auto& actorPtr : world->GetObjectManager()->GetAllActors()) {
    AActor* actor = actorPtr.get();
    if (!actor || actor->IsPendingDestroy()) continue;
    if (actor == gameModeActor || actor->IsEditorActor()) continue;
    const std::string name = actor->GetActorClassName();
    if (!registry.Contains(name)) continue;
    const auto& gameModeClassNames = registry.GetGameModeClassNames();
    if (std::find(gameModeClassNames.begin(), gameModeClassNames.end(), name) !=
        gameModeClassNames.end())
      continue;
    FActorSaveData data;
    data.ClassName = name;
    data.Location = actor->GetActorLocation();
    data.Rotation = actor->GetActorRotation();
    data.Scale = actor->GetActorScale();
    if (auto spriteActor = dynamic_cast<ASpriteActor*>(actor)) {
      data.CustomProperties["ImagePath"] = spriteActor->GetImagePath();
    }
    actors.push_back(data);
  }
  FLevelMetaData meta;
  meta.GameModeClassName = gameModeClassName;
  return SaveData(filePath, meta, actors);
}

bool LevelSerializer::Load(World* world, const std::string& filePath) {
  return Load(world, filePath, true, nullptr);
}

bool LevelSerializer::Load(
    World* world, const std::string& filePath, bool bLoadGameMode, FLevelMetaData* outMeta
) {
  if (!world) {
    return false;
  }
  FLevelMetaData meta;
  std::vector<FActorSaveData> actors;
  if (!LoadData(filePath, meta, actors)) return false;
  if (outMeta) {
    *outMeta = meta;
  }
  M_LOG("Loaded actor count: {}", actors.size());
  auto& registry = ActorRegistry::GetInstance();
  std::vector<AActor*> spawnedActors;

  if (bLoadGameMode && world->IsServer() && !meta.GameModeClassName.empty()) {
    M_LOG("Spawning GameMode: {}", meta.GameModeClassName);
    AActor* gameModeActor = registry.Spawn(world, meta.GameModeClassName);
    AGameModeBase* gameMode = dynamic_cast<AGameModeBase*>(gameModeActor);
    if (gameMode) {
      world->SetGameMode(gameMode);
      spawnedActors.push_back(gameMode);
    } else {
      M_LOG("GameMode spawn failed or class is not AGameModeBase: {}", meta.GameModeClassName);
      if (gameModeActor) {
        gameModeActor->Destroy();
      }
    }
  }

  for (const auto& data : actors) {
    AActor* actor = registry.Spawn(world, data.ClassName, data.Location, data.Rotation);
    if (!actor) {
      M_LOG("Spawn failed: {} (not registered?)", data.ClassName);
      continue;
    }
    actor->SetActorScale(data.Scale);
    if (auto spriteActor = dynamic_cast<ASpriteActor*>(actor)) {
      auto it = data.CustomProperties.find("ImagePath");
      if (it != data.CustomProperties.end()) {
        spriteActor->SetImagePath(it->second);
      }
    }
    spawnedActors.push_back(actor);
  }
  if (!world->IsSimulating()) {
    return true;
  }
  for (auto* actor : spawnedActors) {
    if (actor) actor->Spawned();
  }
  return true;
}

bool LevelSerializer::SaveData(
    const std::string& filePath,
    const FLevelMetaData& meta,
    const std::vector<FActorSaveData>& actors
) {
  json root;
  root["meta"] = json::object();
  root["meta"]["game_mode"] = meta.GameModeClassName;
  json arr = json::array();
  for (const auto& d : actors) {
    json obj;
    obj["class"] = d.ClassName;
    obj["instance_name"] = d.InstanceName;
    obj["transform"] = {
        {"location", {{"x", d.Location.X}, {"y", d.Location.Y}}},
        {"rotation", d.Rotation.Rotation},
        {"scale", d.Scale.Scale}
    };
    if (!d.CustomProperties.empty()) {
      obj["properties"] = d.CustomProperties;
    }
    arr.push_back(obj);
  }
  root["actors"] = arr;
  const std::string encryptedData = SimpleCrypto::Process(root.dump());
  std::ofstream ofs(filePath, std::ios::binary);
  if (!ofs.is_open()) return false;
  ofs.write(encryptedData.data(), static_cast<std::streamsize>(encryptedData.size()));
  return true;
}

bool LevelSerializer::LoadData(
    const std::string& filePath, std::vector<FActorSaveData>& outActors
) {
  FLevelMetaData meta;
  return LoadData(filePath, meta, outActors);
}

bool LevelSerializer::LoadData(
    const std::string& filePath, FLevelMetaData& outMeta, std::vector<FActorSaveData>& outActors
) {
  outMeta = FLevelMetaData{};
  outActors.clear();
  std::ifstream ifs(filePath, std::ios::binary);
  if (!ifs.is_open()) return false;
  const std::string encryptedData(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()
  );
  const std::string decryptedData = SimpleCrypto::Process(encryptedData);
  json root;
  try {
    root = json::parse(decryptedData);
  } catch (const json::exception& e) {
    M_LOG("Level data load failed: tampered or corrupted data. {}", e.what());
    return false;
  }
  if (root.contains("meta") && root["meta"].is_object()) {
    outMeta.GameModeClassName = root["meta"].value("game_mode", "");
  }
  if (!root.contains("actors") || !root["actors"].is_array()) return false;
  for (const auto& obj : root["actors"]) {
    FActorSaveData data;
    data.ClassName = obj.value("class", "");
    data.InstanceName = obj.value("instance_name", "");
    if (obj.contains("transform")) {
      const auto& t = obj["transform"];
      if (t.contains("location")) {
        data.Location.X = t["location"].value("x", 0.0f);
        data.Location.Y = t["location"].value("y", 0.0f);
      }
      data.Rotation = FRotator(t.value("rotation", 0.0f));
      data.Scale = FScale(t.value("scale", 1.0f));
    }
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
