#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "UMath.h"

using FPropertyValue = std::variant<bool, int, float, std::string, FVector2D, FVector3D>;

enum class EPropertyType { Bool, Int, Float, String, Vector2D, Vector3D };

struct FEditorPropertyMetadata {
  std::optional<int> IntMin;
  std::optional<int> IntMax;
  std::optional<int> IntSliderMin;
  std::optional<int> IntSliderMax;
  std::optional<float> FloatMin;
  std::optional<float> FloatMax;
  std::optional<float> FloatSliderMin;
  std::optional<float> FloatSliderMax;
  std::optional<std::size_t> MaxLength;
};

struct FProperty {
  std::string Name;
  EPropertyType Type;
  FPropertyValue (*Get)(const void* Object) = nullptr;
  bool (*Set)(void* Object, const FPropertyValue& Value) = nullptr;
  FEditorPropertyMetadata EditorMetadata;
};

struct BROCCOLI_ENGINE_API FClass {
  std::string Name;
  const FClass* BaseClass = nullptr;
  std::vector<FProperty> OwnProperties;

  std::vector<const FProperty*> GetProperties() const;
  const FProperty* FindProperty(std::string_view PropertyName) const;
};

class BROCCOLI_ENGINE_API FReflectionRegistry {
 public:
  using FToken = std::size_t;
  static FReflectionRegistry& GetInstance();
  FToken Register(FClass Class, std::string ModuleOwner);
  bool Unregister(FToken Token);
  bool UnregisterModule(std::string_view ModuleOwner);
  const FClass* FindClass(std::string_view ClassName) const;
  bool HasModule(std::string_view ModuleOwner) const;

 private:
  FReflectionRegistry();
  ~FReflectionRegistry();
  struct FImpl;
  FImpl* Impl = nullptr;
};

BROCCOLI_ENGINE_API bool IsValidUnicodeScalarString(std::string_view Value, std::size_t MaxLength);
