#include "EditorClipboard.h"

#include <cstdint>
#include <limits>
#include <type_traits>

#include "Actor.h"
#include "ActorRegistry.h"
#include "Log.h"
#include "Reflection.h"
#include "SpriteActor.h"
#include "StaticMeshActor.h"
#include "World.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace {
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

bool EditorClipboard::Copy(AActor* Actor) {
  if (Actor == nullptr || Actor->IsPendingDestroy()) {
    M_LOG(Log, "Copy failed: No actor selected.");
    return false;
  }

  ClipboardData.ClassName = Actor->GetActorClassName();
  ClipboardData.Transform = Actor->GetActorTransform3D();
  ClipboardData.CustomProperties.clear();

  if (auto* SpriteActor = dynamic_cast<ASpriteActor*>(Actor)) {
    ClipboardData.CustomProperties["ImagePath"] = SpriteActor->GetImagePath();
  }
  if (auto* StaticMeshActor = dynamic_cast<AStaticMeshActor*>(Actor)) {
    ClipboardData.CustomProperties["ModelPath"] = StaticMeshActor->GetModelPath();
  }
  if (const FClass* Class = FReflectionRegistry::GetInstance().FindClass(ClipboardData.ClassName)) {
    for (const FProperty* Property : Class->GetProperties()) {
      if (Property->Name == "ImagePath" || Property->Name == "ModelPath") continue;
      ClipboardData.CustomProperties[Property->Name] = ValueToJson(Property->Get(Actor));
    }
  }

  bHasClipboard = true;
  M_LOG(
      Log,
      "Copied Actor: {} at ({}, {})",
      ClipboardData.ClassName,
      ClipboardData.Transform.Location.X,
      ClipboardData.Transform.Location.Y
  );
  return true;
}

AActor* EditorClipboard::Paste(World* WorldPtr, const FVector2D& PasteLocation) {
  return Paste(
      WorldPtr, FVector3D{PasteLocation.X, PasteLocation.Y, ClipboardData.Transform.Location.Z}
  );
}

AActor* EditorClipboard::Paste(World* WorldPtr, const FVector3D& PasteLocation) {
  if (!bHasClipboard) {
    M_LOG(Log, "Paste failed: Clipboard is empty.");
    return nullptr;
  }
  if (WorldPtr == nullptr) {
    M_LOG(Log, "Paste failed: World is null.");
    return nullptr;
  }

  AActor* NewActor = ActorRegistry::GetInstance().Spawn(WorldPtr, ClipboardData.ClassName);
  if (NewActor == nullptr) {
    M_LOG(Log, "Paste failed: Could not spawn actor '{}'.", ClipboardData.ClassName);
    return nullptr;
  }

  ClipboardData.Transform.Location = PasteLocation;
  NewActor->SetActorLocation3D(ClipboardData.Transform.Location);
  NewActor->SetActorRotation3D(ClipboardData.Transform.Rotation);
  NewActor->SetActorScale3D(ClipboardData.Transform.Scale);

  if (auto* SpriteActor = dynamic_cast<ASpriteActor*>(NewActor)) {
    auto It = ClipboardData.CustomProperties.find("ImagePath");
    if (It != ClipboardData.CustomProperties.end()) {
      if (It->second.is_string()) SpriteActor->SetImagePath(It->second.get<std::string>());
    }
  }
  if (auto* StaticMeshActor = dynamic_cast<AStaticMeshActor*>(NewActor)) {
    auto It = ClipboardData.CustomProperties.find("ModelPath");
    if (It != ClipboardData.CustomProperties.end()) {
      if (It->second.is_string()) StaticMeshActor->SetModelPath(It->second.get<std::string>());
    }
  }

  const FClass* Class = FReflectionRegistry::GetInstance().FindClass(ClipboardData.ClassName);
  for (const auto& [Name, JsonValue] : ClipboardData.CustomProperties) {
    if (Name == "ImagePath" || Name == "ModelPath") continue;
    const FProperty* Property = Class != nullptr ? Class->FindProperty(Name) : nullptr;
    if (Property == nullptr) {
      M_LOG(
          Warning, "Unknown clipboard property '{}' on actor '{}'.", Name, ClipboardData.ClassName
      );
      continue;
    }
    FPropertyValue Value;
    if (!JsonToValue(JsonValue, Property->Type, Value) || !Property->Set(NewActor, Value)) {
      M_LOG(
          Warning, "Invalid clipboard property '{}' on actor '{}'.", Name, ClipboardData.ClassName
      );
    }
  }

  M_LOG(
      Log,
      "Pasted Actor: {} at ({}, {}, {})",
      ClipboardData.ClassName,
      PasteLocation.X,
      PasteLocation.Y,
      PasteLocation.Z
  );
  return NewActor;
}

void EditorClipboard::Clear() {
  ClipboardData = FActorSaveData{};
  bHasClipboard = false;
}
