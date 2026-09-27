#include "LevelSerializer.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <type_traits>

#include "Actor.h"
#include "ActorManager.h"
#include "ActorRegistry.h"
#include "EditorSelectPointComponent.h"
#include "GameModeBase.h"
#include "Log.h"
#include "PathResolver.h"
#include "Reflection.h"
#include "SimpleCrypto.h"
#include "SpriteActor.h"
#include "StaticMeshActor.h"
#include "World.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace {
std::string GetLowercaseExtension(const std::string& FilePath) {
  std::string Extension = std::filesystem::path(FilePath).extension().string();
  std::transform(
      Extension.begin(), Extension.end(), Extension.begin(), [](unsigned char Character) {
        return static_cast<char>(std::tolower(Character));
      }
  );
  return Extension;
}

std::string GetLowercaseFileName(const std::string& FilePath) {
  std::string FileName = std::filesystem::path(FilePath).filename().string();
  std::transform(FileName.begin(), FileName.end(), FileName.begin(), [](unsigned char Character) {
    return static_cast<char>(std::tolower(Character));
  });
  return FileName;
}

json ValueToJson(const FPropertyValue& Value) {
  return std::visit(
      [](const auto& Item) -> json {
        using T = std::decay_t<decltype(Item)>;
        if constexpr (std::is_same_v<T, FVector2D>) {
          return {{"x", Item.X}, {"y", Item.Y}};
        } else if constexpr (std::is_same_v<T, FVector3D>) {
          return {{"x", Item.X}, {"y", Item.Y}, {"z", Item.Z}};
        } else {
          return Item;
        }
      },
      Value
  );
}

bool JsonToValue(const json& JsonValue, EPropertyType Type, FPropertyValue& Value) {
  try {
    switch (Type) {
      case EPropertyType::Bool:
        if (!JsonValue.is_boolean()) return false;
        Value = JsonValue.get<bool>();
        return true;
      case EPropertyType::Int:
        if (!JsonValue.is_number_integer()) return false;
        if (JsonValue.is_number_unsigned()) {
          if (JsonValue.get<std::uint64_t>() >
              static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
            return false;
        } else {
          const std::int64_t Number = JsonValue.get<std::int64_t>();
          if (Number < std::numeric_limits<int>::min() || Number > std::numeric_limits<int>::max())
            return false;
        }
        Value = JsonValue.get<int>();
        return true;
      case EPropertyType::Float:
        if (!JsonValue.is_number_float()) return false;
        Value = JsonValue.get<float>();
        return true;
      case EPropertyType::String:
        if (!JsonValue.is_string()) return false;
        Value = JsonValue.get<std::string>();
        return true;
      case EPropertyType::Vector2D:
        if (!JsonValue.is_object() || !JsonValue.contains("x") || !JsonValue.contains("y") ||
            JsonValue.size() != 2 || !JsonValue["x"].is_number_float() ||
            !JsonValue["y"].is_number_float())
          return false;
        Value = FVector2D{JsonValue["x"].get<float>(), JsonValue["y"].get<float>()};
        return true;
      case EPropertyType::Vector3D:
        if (!JsonValue.is_object() || !JsonValue.contains("x") || !JsonValue.contains("y") ||
            !JsonValue.contains("z") || JsonValue.size() != 3 ||
            !JsonValue["x"].is_number_float() || !JsonValue["y"].is_number_float() ||
            !JsonValue["z"].is_number_float())
          return false;
        Value = FVector3D{
            JsonValue["x"].get<float>(), JsonValue["y"].get<float>(), JsonValue["z"].get<float>()
        };
        return true;
    }
  } catch (const json::exception&) {
    return false;
  }
  return false;
}
}  // namespace

bool LevelSerializer::Save(
    World* world, const std::string& filePath, const std::string& gameModeClassName
) {
  if (!world) {
    return false;
  }
  std::vector<FActorSaveData> actors;
  auto& registry = ActorRegistry::GetInstance();
  AActor* gameModeActor = world->GetGameMode();
  for (const auto& actorPtr : world->GetActorManager()->GetAllActors()) {
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
    data.InstanceName = actor->GetInstanceName();
    data.Transform = actor->GetActorTransform3D();
    if (auto spriteActor = dynamic_cast<ASpriteActor*>(actor)) {
      data.CustomProperties["ImagePath"] =
          PathResolver::SanitizeResourcePath(spriteActor->GetImagePath());
    }
    if (auto staticMeshActor = dynamic_cast<AStaticMeshActor*>(actor)) {
      data.CustomProperties["ModelPath"] =
          PathResolver::SanitizeResourcePath(staticMeshActor->GetModelPath());
    }
    if (const FClass* Class = FReflectionRegistry::GetInstance().FindClass(name)) {
      for (const FProperty* Property : Class->GetProperties()) {
        if (Property->Name == "ImagePath" || Property->Name == "ModelPath") continue;
        data.CustomProperties[Property->Name] = ValueToJson(Property->Get(actor));
      }
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
  M_LOG(Log, "Loaded actor count: {}", actors.size());
  auto& registry = ActorRegistry::GetInstance();
  std::vector<AActor*> spawnedActors;

  if (bLoadGameMode && world->IsServer() && !meta.GameModeClassName.empty()) {
    M_LOG(Log, "Spawning GameMode: {}", meta.GameModeClassName);
    AActor* gameModeActor = registry.Spawn(world, meta.GameModeClassName);
    AGameModeBase* gameMode = dynamic_cast<AGameModeBase*>(gameModeActor);
    if (gameMode) {
      world->SetGameMode(gameMode);
      spawnedActors.push_back(gameMode);
    } else {
      M_LOG(Log, "GameMode spawn failed or class is not AGameModeBase: {}", meta.GameModeClassName);
      if (gameModeActor) {
        gameModeActor->Destroy();
      }
    }
  }

  for (const auto& data : actors) {
    AActor* actor = registry.Spawn(world, data.ClassName);
    if (!actor) {
      M_LOG(Log, "Spawn failed: {} (not registered?)", data.ClassName);
      continue;
    }

    if (world->IsClient() && actor->bReplicates) {
      actor->Destroy();
      continue;
    }

    if (!data.InstanceName.empty()) {
      world->GetActorManager()->AssignInstanceName(*actor, data.InstanceName);
    }

    actor->SetActorLocation3D(data.Transform.Location);
    actor->SetActorRotation3D(data.Transform.Rotation);
    actor->SetActorScale3D(data.Transform.Scale);
    if (auto spriteActor = dynamic_cast<ASpriteActor*>(actor)) {
      auto it = data.CustomProperties.find("ImagePath");
      if (it != data.CustomProperties.end()) {
        if (it->second.is_string()) spriteActor->SetImagePath(it->second.get<std::string>());
      }
    }
    if (auto staticMeshActor = dynamic_cast<AStaticMeshActor*>(actor)) {
      auto it = data.CustomProperties.find("ModelPath");
      if (it != data.CustomProperties.end()) {
        if (it->second.is_string()) staticMeshActor->SetModelPath(it->second.get<std::string>());
      }
    }
    const FClass* Class = FReflectionRegistry::GetInstance().FindClass(data.ClassName);
    for (const auto& [Name, JsonValue] : data.CustomProperties) {
      if (Name == "ImagePath" || Name == "ModelPath") continue;
      const FProperty* Property = Class != nullptr ? Class->FindProperty(Name) : nullptr;
      if (Property == nullptr) {
        M_LOG(Warning, "Unknown level property '{}' on actor '{}'.", Name, data.ClassName);
        continue;
      }
      FPropertyValue Value;
      if (!JsonToValue(JsonValue, Property->Type, Value) || !Property->Set(actor, Value)) {
        M_LOG(Warning, "Invalid level property '{}' on actor '{}'.", Name, data.ClassName);
      }
    }
    spawnedActors.push_back(actor);
  }
  world->GetActorManager()->FlushPendingActors();
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
  root["meta"]["format_version"] = 3;
  root["meta"]["game_mode"] = meta.GameModeClassName;
  json arr = json::array();
  for (const auto& d : actors) {
    json obj;
    obj["class"] = d.ClassName;
    obj["instance_name"] = d.InstanceName;
    obj["transform"] = {
        {"location",
         {{"x", d.Transform.Location.X},
          {"y", d.Transform.Location.Y},
          {"z", d.Transform.Location.Z}}},
        {"rotation",
         {{"x", d.Transform.Rotation.X},
          {"y", d.Transform.Rotation.Y},
          {"z", d.Transform.Rotation.Z},
          {"w", d.Transform.Rotation.W}}},
        {"scale",
         {{"x", d.Transform.Scale.X}, {"y", d.Transform.Scale.Y}, {"z", d.Transform.Scale.Z}}}
    };
    if (!d.CustomProperties.empty()) {
      obj["properties"] = d.CustomProperties;
    }
    arr.push_back(obj);
  }
  root["actors"] = arr;
  const std::string JsonData = root.dump(2);
  const std::string Extension = GetLowercaseExtension(filePath);
  const bool IsLevelJson = GetLowercaseFileName(filePath).ends_with(".blevel.json");
  std::string DataToWrite;
  if (IsLevelJson) {
    DataToWrite = JsonData;
  } else if (Extension == ".blevel") {
    DataToWrite = SimpleCrypto::Process(JsonData);
  } else {
    M_LOG(Log, "Level data save failed: unsupported file extension '{}'.", Extension);
    return false;
  }

  std::ofstream ofs(filePath, std::ios::binary);
  if (!ofs.is_open()) {
    M_LOG(Log, "Level data save failed: could not open '{}'.", filePath);
    return false;
  }
  ofs.write(DataToWrite.data(), static_cast<std::streamsize>(DataToWrite.size()));
  return ofs.good();
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
  if (!ifs.is_open()) {
    M_LOG(Log, "Level data load failed: could not open '{}'.", filePath);
    return false;
  }
  const std::string FileData(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()
  );
  const std::string Extension = GetLowercaseExtension(filePath);
  const bool IsLevelJson = GetLowercaseFileName(filePath).ends_with(".blevel.json");
  std::string JsonData;
  if (IsLevelJson) {
    JsonData = FileData;
  } else if (Extension == ".blevel") {
    JsonData = SimpleCrypto::Process(FileData);
  } else {
    M_LOG(Log, "Level data load failed: unsupported file extension '{}'.", Extension);
    return false;
  }

  json root;
  try {
    root = json::parse(JsonData);
  } catch (const json::exception& e) {
    M_LOG(Log, "Level data load failed: invalid or corrupted JSON. {}", e.what());
    return false;
  }
  int FormatVersion = 1;
  if (root.contains("meta")) {
    const auto& Meta = root["meta"];
    if (!Meta.is_object()) {
      return false;
    }
    outMeta.GameModeClassName = Meta.value("game_mode", "");
    if (Meta.contains("format_version")) {
      if (!Meta["format_version"].is_number_integer()) {
        return false;
      }
      FormatVersion = Meta["format_version"].get<int>();
    }
  }
  if (FormatVersion != 1 && FormatVersion != 2 && FormatVersion != 3) {
    M_LOG(Error, "Level data load failed: unsupported format version {}.", FormatVersion);
    return false;
  }
  if (!root.contains("actors") || !root["actors"].is_array()) return false;
  for (const auto& obj : root["actors"]) {
    FActorSaveData data;
    data.ClassName = obj.value("class", "");
    data.InstanceName = obj.value("instance_name", "");
    if (obj.contains("transform")) {
      const auto& t = obj["transform"];
      if (t.contains("location")) {
        data.Transform.Location.X = t["location"].value("x", 0.0f);
        data.Transform.Location.Y = t["location"].value("y", 0.0f);
        data.Transform.Location.Z = t["location"].value("z", 0.0f);
      }
      if (FormatVersion >= 2 && t.contains("rotation") && t["rotation"].is_object()) {
        data.Transform.Rotation = {
            t["rotation"].value("x", 0.0f),
            t["rotation"].value("y", 0.0f),
            t["rotation"].value("z", 0.0f),
            t["rotation"].value("w", 1.0f)
        };
        data.Transform.Rotation = data.Transform.Rotation.Normalize();
        if (t.contains("scale") && t["scale"].is_object()) {
          data.Transform.Scale = {
              t["scale"].value("x", 1.0f), t["scale"].value("y", 1.0f), t["scale"].value("z", 1.0f)
          };
        }
      } else {
        data.Transform.Rotation = FQuaternion::FromRotator({0.0f, t.value("rotation", 0.0f), 0.0f});
        data.Transform.Scale = FScale3D(t.value("scale", 1.0f));
      }
    }
    if (obj.contains("properties")) {
      for (auto& [key, val] : obj["properties"].items()) {
        data.CustomProperties[key] = val;
      }
    }
    outActors.push_back(data);
  }
  return true;
}
