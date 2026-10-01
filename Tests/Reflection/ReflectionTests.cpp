#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "Actor.h"
#include "ActorRegistry.h"
#include "DirectionalLightActor.h"
#include "DirectionalLightComponent.h"
#include "EditorClipboard.h"
#include "LevelSerializer.h"
#include "Log.h"
#include "PathResolver.h"
#include "PluginHost.h"
#include "ReflectionGenerator.h"
#include "ResourceManager.h"
#include "Sprite2DActor.h"
#include "Sprite2DComponent.h"
#include "Sprite3DActor.h"
#include "Sprite3DComponent.h"
#include "StaticMeshActor.h"
#include "StaticMeshComponent.h"
#include "World.h"
#include "nlohmann/json.hpp"

namespace {
static_assert(
    std::is_same_v<decltype(&ASprite2DActor::SetImagePath), void (ASprite2DActor::*)(const FPath&)>
);
static_assert(std::is_same_v<
              decltype(&ASprite2DActor::GetImagePath),
              const FPath& (ASprite2DActor::*)() const>);
static_assert(
    std::is_same_v<decltype(&ASprite3DActor::SetImagePath), void (ASprite3DActor::*)(const FPath&)>
);
static_assert(std::is_same_v<
              decltype(&ASprite3DActor::GetImagePath),
              const FPath& (ASprite3DActor::*)() const>);
static_assert(std::is_same_v<
              decltype(&AStaticMeshActor::SetModelPath),
              void (AStaticMeshActor::*)(const FPath&)>);
static_assert(std::is_same_v<
              decltype(&AStaticMeshActor::GetModelPath),
              const FPath& (AStaticMeshActor::*)() const>);

void Check(bool Condition, std::string_view Message) {
  if (!Condition) throw std::runtime_error(std::string(Message));
}

const FProperty& RequireProperty(const FClass& Class, std::string_view Name) {
  const FProperty* Property = Class.FindProperty(Name);
  Check(Property != nullptr, "Missing reflected property.");
  return *Property;
}

template <class V>
V ReadProperty(const FClass& Class, std::string_view Name, const void* Object) {
  return std::get<V>(RequireProperty(Class, Name).Get(Object));
}

class FReflectionTestSubject {
 public:
  const FProperty* ClampedValueProperty = nullptr;
  int CallbackCount = 0;
  int LastOldValue = -1;
  bool ReentryAccepted = true;

  void OnClampedValueChanged(int OldValue) {
    ++CallbackCount;
    LastOldValue = OldValue;
    if (ClampedValueProperty != nullptr) ReentryAccepted = ClampedValueProperty->Set(this, int{7});
  }

  void OnThrowingValueChanged(int) { throw std::runtime_error("expected callback failure"); }

  EDITOR_PROPERTY()
  bool Enabled = false;
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^FReflectionTestSubject::OnClampedValueChanged, .Min = 0, .Max = 10,
  )
  int ClampedValue = 5;
  EDITOR_PROPERTY(.OnEditorChanged = ^^FReflectionTestSubject::OnThrowingValueChanged)
  int ThrowingValue = 1;
  EDITOR_PROPERTY(.Min = -1.0F, .Max = 1.0F)
  float Weight = 0.0F;
  EDITOR_PROPERTY(.Min = 0)
  float ZeroBasedWeight = 1.0F;
  EDITOR_PROPERTY(.MaxLength = 3)
  std::string Label = "initial";
  EDITOR_PROPERTY()
  FVector2D Offset{};
  EDITOR_PROPERTY()
  FVector3D Position{};
};

class FBaseReflectionSubject {
 private:
  EDITOR_PROPERTY()
  bool BaseEnabled = false;
};

class FDerivedReflectionSubject : public FBaseReflectionSubject {
 public:
  EDITOR_PROPERTY(.Min = 0, .Max = 4)
  int DerivedValue = 2;
};

class FPrivateReflectionSubject {
 public:
  int CallbackCount = 0;
  int LastOldValue = -1;

 protected:
  void OnLevelChanged(int OldValue) {
    ++CallbackCount;
    LastOldValue = OldValue;
  }

  EDITOR_PROPERTY(.SliderMin = 0, .SliderMax = 1)
  float Gain = 0.5F;

 private:
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^FPrivateReflectionSubject::OnLevelChanged, .Min = 0, .Max = 10
  )
  int Level = 5;

  EDITOR_PROPERTY(.MaxLength = 3)
  std::string Name = "abc";

  int Hidden = 1;
};

class FPrivatePathSubject {
 public:
  int CallbackCount = 0;
  FPath LastOldValue;

 private:
  void OnPathChanged(FPath OldValue) {
    ++CallbackCount;
    LastOldValue = OldValue;
  }
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^FPrivatePathSubject::OnPathChanged, .PathFilter = EPathFilter::Image
  )
  FPath ImagePath;
};

void TestPathValueAndReflection() {
  Check(FPath("Textures/A.png").String() == "/Game/Textures/A.png", "Relative path root failed.");
  Check(FPath("Engine/Fonts/A.ttf").String() == "/Engine/Fonts/A.ttf", "Engine root failed.");
  Check(FPath().Empty(), "Empty path should be valid.");
  Check(!PathResolver::MakeVirtualPath("C:/Outside/A.png"), "External absolute path accepted.");
  Check(!PathResolver::MakeVirtualPath("../Outside/A.png"), "Path traversal accepted.");
  const auto GameFile = std::filesystem::absolute(
      std::filesystem::path(PathResolver::GetGameResourceDir()) / "Textures/A.png"
  );
  const auto EngineFile = std::filesystem::absolute(
      std::filesystem::path(PathResolver::GetEngineResourceDir()) / "Fonts/A.ttf"
  );
  Check(
      FPath(GameFile.string()).String() == "/Game/Textures/A.png",
      "Game resource absolute path was not virtualized."
  );
  Check(
      FPath(EngineFile.string()).String() == "/Engine/Fonts/A.ttf",
      "Engine resource absolute path was not virtualized."
  );

  FClass Class = ReflectionGenerator::MakeClass<FPrivatePathSubject>("PrivatePathSubject");
  const FProperty& Property = RequireProperty(Class, "ImagePath");
  Check(Property.Type == EPropertyType::Path, "FPath type was not inferred.");
  Check(Property.EditorMetadata.PathFilter == EPathFilter::Image, "Path filter was lost.");
  FPrivatePathSubject Subject;
  Check(Property.Set(&Subject, FPath("Textures/A.png")), "Path Set failed.");
  Check(
      std::get<FPath>(Property.Get(&Subject)).String() == "/Game/Textures/A.png", "Path Get failed."
  );
  Check(Subject.CallbackCount == 1 && Subject.LastOldValue.Empty(), "Path callback failed.");
  Check(
      Property.Set(&Subject, FPath("/Game/Textures/A.png")) && Subject.CallbackCount == 1,
      "Unchanged path invoked callback."
  );
}

class AReflectionSerializerTestActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AReflectionSerializerTestActor)

  void OnCountChanged(int OldValue) {
    ++CallbackCount;
    LastOldCount = OldValue;
  }

  void OnAssetChanged(FPath OldValue) {
    ++PathCallbackCount;
    LastOldPath = OldValue;
  }

  int CallbackCount = 0;
  int LastOldCount = -1;
  int PathCallbackCount = 0;
  FPath LastOldPath;

 private:
  EDITOR_PROPERTY()
  bool Enabled = false;
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^AReflectionSerializerTestActor::OnCountChanged, .Min = 0, .Max = 100,
  )
  int Count = 7;
  EDITOR_PROPERTY()
  float Weight = 1.25F;
  EDITOR_PROPERTY()
  std::string Label = "cpp-initial";
  EDITOR_PROPERTY()
  FVector2D Offset{1.0F, 2.0F};
  EDITOR_PROPERTY()
  FVector3D Position{3.0F, 4.0F, 5.0F};
  EDITOR_PROPERTY(.OnEditorChanged = ^^AReflectionSerializerTestActor::OnAssetChanged)
  FPath AssetPath;
};

class AReflectionPluginTestActor final : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AReflectionPluginTestActor)
};

class AReflectionStaticBase : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AReflectionStaticBase)
  int CallbackCount = 0;
  int LastOldValue = -1;

 protected:
  virtual void OnBaseValueChanged(int OldValue) {
    ++CallbackCount;
    LastOldValue = OldValue;
  }

 private:
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^AReflectionStaticBase::OnBaseValueChanged, .Min = 0, .Max = 10
  )
  int BaseValue = 2;
  int HiddenValue = 0;
};

class AReflectionIntermediate : public AReflectionStaticBase {
 protected:
  EDITOR_PROPERTY()
  int IntermediateValue = 1;
};

class AReflectionStaticDerived final : public AReflectionIntermediate {
 public:
  DEFINE_ACTOR_CLASS(AReflectionStaticDerived)
  bool OverrideCalled = false;
  EDITOR_PROPERTY()
  float DerivedValue = 2.0F;

 protected:
  void OnBaseValueChanged(int OldValue) override {
    OverrideCalled = true;
    AReflectionStaticBase::OnBaseValueChanged(OldValue);
  }
};

REGISTER_ACTOR(AReflectionStaticDerived)
REGISTER_ACTOR(AReflectionStaticBase)

void TestNonPublicProperties() {
  FClass Class = ReflectionGenerator::MakeClass<FPrivateReflectionSubject>("PrivateSubject");
  Check(Class.GetProperties().size() == 3, "Non-public property enumeration is incorrect.");
  Check(Class.FindProperty("Hidden") == nullptr, "Unannotated private member was exposed.");
  FPrivateReflectionSubject Subject;

  const FProperty& Level = RequireProperty(Class, "Level");
  Check(
      Level.EditorMetadata.IntMin == 0 && Level.EditorMetadata.IntMax == 10,
      "Private int metadata is missing."
  );
  Check(Level.Set(&Subject, 20), "Private int setter failed.");
  Check(std::get<int>(Level.Get(&Subject)) == 10, "Private int setter did not clamp.");
  Check(
      Subject.CallbackCount == 1 && Subject.LastOldValue == 5,
      "Private callback received the wrong old value."
  );
  Check(
      Level.Set(&Subject, 10) && Subject.CallbackCount == 1,
      "Private callback ran for an unchanged value."
  );

  const FProperty& Gain = RequireProperty(Class, "Gain");
  Check(
      Gain.EditorMetadata.FloatSliderMin == 0.0F && Gain.EditorMetadata.FloatSliderMax == 1.0F,
      "Protected float slider metadata is missing."
  );
  Check(Gain.Set(&Subject, 2.0F), "Protected float setter failed.");
  Check(
      std::get<float>(Gain.Get(&Subject)) == 2.0F, "Slider limits incorrectly clamped the value."
  );

  const FProperty& Name = RequireProperty(Class, "Name");
  Check(Name.EditorMetadata.MaxLength == 3, "Private string MaxLength is missing.");
  Check(!Name.Set(&Subject, std::string("long")), "Private string setter ignored MaxLength.");
  Check(Name.Set(&Subject, std::string("new")), "Private string setter rejected a valid value.");
  Check(std::get<std::string>(Name.Get(&Subject)) == "new", "Private string getter failed.");
}

void TestStaticActorReflectionRegistration() {
  const FClass* Derived =
      FReflectionRegistry::GetInstance().FindClass(AReflectionStaticDerived::StaticClassName());
  Check(Derived != nullptr, "Static derived Actor has no Reflection class.");
  Check(Derived->GetProperties().size() == 3, "Static Actor Reflection inheritance is incomplete.");
  const FProperty& BaseValue = RequireProperty(*Derived, "BaseValue");
  Check(Derived->FindProperty("IntermediateValue") != nullptr, "Intermediate property is missing.");
  Check(Derived->FindProperty("DerivedValue") != nullptr, "Derived property is missing.");
  Check(Derived->FindProperty("HiddenValue") == nullptr, "Unannotated private member was exposed.");
  AReflectionStaticDerived Subject;
  Check(BaseValue.Set(&Subject, 20), "Base private property could not be set on a derived Actor.");
  Check(
      std::get<int>(BaseValue.Get(&Subject)) == 10,
      "Base private property did not clamp on a derived Actor."
  );
  Check(
      Subject.OverrideCalled && Subject.CallbackCount == 1 && Subject.LastOldValue == 2,
      "Base private property did not invoke the virtual override with its old value."
  );
  Check(
      ActorRegistry::GetInstance().Contains(AReflectionStaticDerived::StaticClassName()),
      "Static derived Actor factory is missing."
  );
}

void TestSixPropertyTypesAndMetadata() {
  FClass Class = ReflectionGenerator::MakeClass<FReflectionTestSubject>("ReflectionTestSubject");
  Check(Class.OwnProperties.size() == 8, "Unannotated data members were reflected as properties.");
  FReflectionTestSubject Subject;

  const FProperty& Enabled = RequireProperty(Class, "Enabled");
  Check(Enabled.Type == EPropertyType::Bool, "Bool property type is incorrect.");
  Check(Enabled.Set(&Subject, true), "Bool setter rejected a valid value.");
  Check(Subject.Enabled && std::get<bool>(Enabled.Get(&Subject)), "Bool getter/setter failed.");

  const FProperty& ClampedValue = RequireProperty(Class, "ClampedValue");
  Check(ClampedValue.Type == EPropertyType::Int, "Int property type is incorrect.");
  Check(ClampedValue.EditorMetadata.IntMin == 0, "Int minimum metadata is missing.");
  Check(ClampedValue.EditorMetadata.IntMax == 10, "Int maximum metadata is missing.");
  Subject.ClampedValueProperty = &ClampedValue;
  Check(ClampedValue.Set(&Subject, 20), "Int setter rejected a valid clamped value.");
  Check(Subject.ClampedValue == 10, "Int setter did not clamp to Max.");
  Check(
      Subject.CallbackCount == 1 && Subject.LastOldValue == 5,
      "Int callback did not receive the old value."
  );
  Check(!Subject.ReentryAccepted, "Same-object callback reentry was accepted.");
  Check(ClampedValue.Set(&Subject, 20), "Int setter rejected an unchanged value.");
  Check(Subject.CallbackCount == 1, "Callback ran for an unchanged value.");
  Check(ClampedValue.Set(&Subject, -5), "Int setter rejected a valid clamped value.");
  Check(Subject.ClampedValue == 0, "Int setter did not clamp to Min.");

  const FProperty& Weight = RequireProperty(Class, "Weight");
  Check(Weight.Type == EPropertyType::Float, "Float property type is incorrect.");
  Check(Weight.EditorMetadata.FloatMin == -1.0F, "Float minimum metadata is missing.");
  Check(Weight.EditorMetadata.FloatMax == 1.0F, "Float maximum metadata is missing.");
  Check(Weight.Set(&Subject, 4.0F) && Subject.Weight == 1.0F, "Float setter did not clamp to Max.");
  Check(
      !Weight.Set(&Subject, std::numeric_limits<float>::quiet_NaN()), "Float setter accepted NaN."
  );

  const FProperty& ZeroBasedWeight = RequireProperty(Class, "ZeroBasedWeight");
  Check(
      ZeroBasedWeight.EditorMetadata.FloatMin == 0.0F,
      "Integer metadata was not converted for a float property."
  );

  const FProperty& Label = RequireProperty(Class, "Label");
  Check(Label.Type == EPropertyType::String, "String property type is incorrect.");
  Check(Label.EditorMetadata.MaxLength == 3, "String MaxLength metadata is missing.");
  const std::string ThreeUnicodeScalars =
      "\xF0\x9F\x98\x80\xC3\xA9"
      "a";
  Check(
      Label.Set(&Subject, ThreeUnicodeScalars),
      "String limit counted UTF-8 bytes instead of Unicode scalars."
  );
  Check(Subject.Label == ThreeUnicodeScalars, "String getter/setter failed.");
  Check(
      !Label.Set(&Subject, ThreeUnicodeScalars + "b"),
      "String setter accepted more than MaxLength Unicode scalars."
  );
  Check(!Label.Set(&Subject, std::string("\xC0\x80", 2)), "String setter accepted invalid UTF-8.");
  Check(Subject.Label == ThreeUnicodeScalars, "Rejected string input changed the property.");

  const FProperty& Offset = RequireProperty(Class, "Offset");
  Check(Offset.Type == EPropertyType::Vector2D, "Vector2D property type is incorrect.");
  Check(Offset.Set(&Subject, FVector2D{2.0F, 3.0F}), "Vector2D setter rejected a valid value.");
  const FVector2D ReadOffset = std::get<FVector2D>(Offset.Get(&Subject));
  Check(ReadOffset.X == 2.0F && ReadOffset.Y == 3.0F, "Vector2D getter/setter failed.");
  Check(
      !Offset.Set(&Subject, FVector2D{std::numeric_limits<float>::infinity(), 0.0F}),
      "Vector2D setter accepted infinity."
  );

  const FProperty& Position = RequireProperty(Class, "Position");
  Check(Position.Type == EPropertyType::Vector3D, "Vector3D property type is incorrect.");
  Check(
      Position.Set(&Subject, FVector3D{4.0F, 5.0F, 6.0F}), "Vector3D setter rejected a valid value."
  );
  const FVector3D ReadPosition = std::get<FVector3D>(Position.Get(&Subject));
  Check(
      ReadPosition.X == 4.0F && ReadPosition.Y == 5.0F && ReadPosition.Z == 6.0F,
      "Vector3D getter/setter failed."
  );
  Check(
      !Position.Set(&Subject, FVector3D{0.0F, std::numeric_limits<float>::quiet_NaN(), 0.0F}),
      "Vector3D setter accepted NaN."
  );
}

void TestTypeMismatchAndCallbackFailure() {
  FClass Class = ReflectionGenerator::MakeClass<FReflectionTestSubject>("ReflectionFailureSubject");
  FReflectionTestSubject Subject;
  const FProperty& ClampedValue = RequireProperty(Class, "ClampedValue");
  Check(
      !ClampedValue.Set(&Subject, std::string("wrong type")),
      "Setter accepted a value with the wrong variant type."
  );
  Check(Subject.ClampedValue == 5, "Type mismatch changed the property value.");
  Check(!ClampedValue.Set(nullptr, 8), "Setter accepted a null object.");

  const FProperty& ThrowingValue = RequireProperty(Class, "ThrowingValue");
  Check(!ThrowingValue.Set(&Subject, 9), "Setter reported success after its callback threw.");
  Check(Subject.ThrowingValue == 9, "Callback failure rolled back the assigned value.");

  MLog::Flush();
  FLogQuery Query;
  Query.MinimumLevel = ELogLevel::Error;
  const FLogQueryResult Logs = MLog::GetRecentEntries(Query);
  const bool HasCallbackError = std::ranges::any_of(Logs.Entries, [](const FLogEntry& Entry) {
    return Entry.Message.find("Reflection Set callback failed: expected callback failure") !=
           std::string::npos;
  });
  Check(HasCallbackError, "Callback exception was not recorded in the engine log.");
}

void TestInheritanceAndRegistry() {
  FReflectionRegistry& Registry = FReflectionRegistry::GetInstance();
  Registry.UnregisterModule("ReflectionTests");
  FClass Base = ReflectionGenerator::MakeClass<FBaseReflectionSubject>("ReflectionTestBase");
  const FReflectionRegistry::FToken BaseToken = Registry.Register(Base, "ReflectionTests");
  Check(BaseToken != 0, "Base class registration failed.");
  const FClass* RegisteredBase = Registry.FindClass("ReflectionTestBase");
  Check(RegisteredBase != nullptr, "Registered base class lookup failed.");

  FClass Derived = ReflectionGenerator::MakeClass<FDerivedReflectionSubject>(
      "ReflectionTestDerived", RegisteredBase
  );
  const FReflectionRegistry::FToken DerivedToken = Registry.Register(Derived, "ReflectionTests");
  Check(DerivedToken != 0, "Derived class registration failed.");
  Check(Registry.HasModule("ReflectionTests"), "Module registration was not recorded.");

  FDerivedReflectionSubject Subject;
  const FProperty& BaseEnabled = RequireProperty(Derived, "BaseEnabled");
  const FProperty& DerivedValue = RequireProperty(Derived, "DerivedValue");
  Check(
      BaseEnabled.Set(&Subject, true) && std::get<bool>(BaseEnabled.Get(&Subject)),
      "Inherited property could not be set on the derived object."
  );
  Check(
      DerivedValue.Set(&Subject, 6) && Subject.DerivedValue == 4, "Derived property clamp failed."
  );
  Check(Derived.GetProperties().size() == 2, "Inherited property enumeration is incorrect.");

  Registry.UnregisterModule("ReflectionTests");
  Check(
      !Registry.HasModule("ReflectionTests"), "Module unregister left reflection classes behind."
  );
  Check(
      Registry.FindClass("ReflectionTestBase") == nullptr,
      "Module unregister left the base class registered."
  );
  Check(
      Registry.FindClass("ReflectionTestDerived") == nullptr,
      "Module unregister left the derived class registered."
  );
}

void WriteJson(const std::filesystem::path& Path, const nlohmann::json& Value) {
  std::ofstream Output(Path, std::ios::binary);
  Check(Output.is_open(), "Could not create a temporary level file.");
  Output << Value.dump(2);
  Check(Output.good(), "Could not write a temporary level file.");
}

void TestLevelSerializerRoundTripAndLegacyVersions() {
  namespace fs = std::filesystem;
  using json = nlohmann::json;
  const fs::path TempDirectory =
      fs::temp_directory_path() /
      ("BroccoliReflectionTests-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(TempDirectory);

  const fs::path RoundTripPath = TempDirectory / "RoundTrip.BLevel.json";
  FActorSaveData Actor;
  Actor.ClassName = "ReflectionRoundTripActor";
  Actor.InstanceName = "RoundTripActor";
  Actor.CustomProperties = {
      {"Enabled", true},
      {"Count", 42},
      {"Weight", 0.75},
      {"Label", "Unicode \xF0\x9F\x98\x80"},
      {"Offset", {{"x", 1.25}, {"y", -2.5}}},
      {"Position", {{"x", 3.0}, {"y", 4.5}, {"z", -6.0}}},
  };
  const FLevelMetaData Meta{"ReflectionGameMode"};
  Check(
      LevelSerializer::SaveData(RoundTripPath.string(), Meta, {Actor}),
      "LevelSerializer::SaveData failed."
  );

  std::ifstream SavedInput(RoundTripPath, std::ios::binary);
  Check(SavedInput.is_open(), "Could not read the saved level file.");
  const json Saved = json::parse(SavedInput);
  SavedInput.close();
  Check(Saved.at("meta").at("format_version") == 3, "SaveData did not emit format version 3.");
  const json& SavedProperties = Saved.at("actors").at(0).at("properties");
  Check(
      SavedProperties.at("Enabled").is_boolean() && SavedProperties.at("Enabled") == true,
      "Saved bool property changed type or value."
  );
  Check(
      SavedProperties.at("Count").is_number_integer() && SavedProperties.at("Count") == 42,
      "Saved int property changed type or value."
  );
  Check(
      SavedProperties.at("Weight").is_number_float() && SavedProperties.at("Weight") == 0.75,
      "Saved float property changed type or value."
  );
  Check(
      SavedProperties.at("Label").is_string() &&
          SavedProperties.at("Label") == Actor.CustomProperties.at("Label"),
      "Saved string property changed type or value."
  );
  Check(
      SavedProperties.at("Offset") == Actor.CustomProperties.at("Offset"),
      "Saved Vector2D JSON value changed."
  );
  Check(
      SavedProperties.at("Position") == Actor.CustomProperties.at("Position"),
      "Saved Vector3D JSON value changed."
  );

  FLevelMetaData LoadedMeta;
  std::vector<FActorSaveData> LoadedActors;
  Check(
      LevelSerializer::LoadData(RoundTripPath.string(), LoadedMeta, LoadedActors),
      "LoadData failed for a version 3 file."
  );
  Check(
      LoadedMeta.GameModeClassName == Meta.GameModeClassName && LoadedActors.size() == 1,
      "Version 3 metadata or actor count changed after loading."
  );
  Check(
      LoadedActors[0].CustomProperties == Actor.CustomProperties,
      "Version 3 custom property JSON values did not round trip."
  );

  const fs::path Version1Path = TempDirectory / "LegacyV1.BLevel.json";
  WriteJson(
      Version1Path,
      json{
          {"actors",
           json::array({json{
               {"class", "LegacyActor"},
               {"instance_name", "LegacyV1"},
               {"transform",
                {{"location", {{"x", 1.0}, {"y", 2.0}, {"z", 3.0}}},
                 {"rotation", 45.0},
                 {"scale", 2.0}}},
           }})}
      }
  );
  Check(
      LevelSerializer::LoadData(Version1Path.string(), LoadedMeta, LoadedActors),
      "LoadData failed for a version 1 file."
  );
  Check(
      LoadedMeta.GameModeClassName.empty() && LoadedActors.size() == 1,
      "Version 1 default metadata or actor count is incorrect."
  );
  Check(
      LoadedActors[0].Transform.Location.X == 1.0F &&
          LoadedActors[0].Transform.Location.Y == 2.0F &&
          LoadedActors[0].Transform.Location.Z == 3.0F,
      "Version 1 location did not load."
  );
  Check(
      LoadedActors[0].Transform.Scale.X == 2.0F && LoadedActors[0].Transform.Scale.Y == 2.0F &&
          LoadedActors[0].Transform.Scale.Z == 2.0F,
      "Version 1 uniform scale did not load."
  );

  const fs::path Version2Path = TempDirectory / "LegacyV2.BLevel.json";
  WriteJson(
      Version2Path,
      json{
          {"meta", {{"format_version", 2}, {"game_mode", "LegacyGameMode"}}},
          {"actors",
           json::array({json{
               {"class", "LegacyActor"},
               {"instance_name", "LegacyV2"},
               {"transform",
                {{"location", {{"x", 4.0}, {"y", 5.0}, {"z", 6.0}}},
                 {"rotation", {{"x", 0.0}, {"y", 0.7071068}, {"z", 0.0}, {"w", 0.7071068}}},
                 {"scale", {{"x", 2.0}, {"y", 3.0}, {"z", 4.0}}}}},
           }})}
      }
  );
  Check(
      LevelSerializer::LoadData(Version2Path.string(), LoadedMeta, LoadedActors),
      "LoadData failed for a version 2 file."
  );
  Check(
      LoadedMeta.GameModeClassName == "LegacyGameMode" && LoadedActors.size() == 1,
      "Version 2 metadata or actor count is incorrect."
  );
  Check(
      LoadedActors[0].Transform.Location.X == 4.0F &&
          LoadedActors[0].Transform.Location.Y == 5.0F &&
          LoadedActors[0].Transform.Location.Z == 6.0F,
      "Version 2 location did not load."
  );
  Check(
      LoadedActors[0].Transform.Scale.X == 2.0F && LoadedActors[0].Transform.Scale.Y == 3.0F &&
          LoadedActors[0].Transform.Scale.Z == 4.0F,
      "Version 2 vector scale did not load."
  );
  Check(
      std::abs(LoadedActors[0].Transform.Rotation.Y - 0.7071068F) < 0.0001F,
      "Version 2 quaternion did not load and normalize."
  );
  const fs::path TempRoot = fs::weakly_canonical(fs::temp_directory_path());
  const fs::path CanonicalTempDirectory = fs::weakly_canonical(TempDirectory);
  Check(
      CanonicalTempDirectory.is_absolute() && CanonicalTempDirectory.parent_path() == TempRoot &&
          CanonicalTempDirectory.filename().string().starts_with("BroccoliReflectionTests-"),
      "Refusing to remove a level-test directory outside its dedicated temp root."
  );
  fs::remove_all(CanonicalTempDirectory);
}

AReflectionSerializerTestActor* FindSerializerTestActor(World& TargetWorld) {
  for (const std::unique_ptr<AActor>& Actor : TargetWorld.GetActorManager()->GetAllActors()) {
    if (auto* TestActor = dynamic_cast<AReflectionSerializerTestActor*>(Actor.get()))
      return TestActor;
  }
  return nullptr;
}

void TestWorldLevelSerializerReflectionRoundTrip() {
  namespace fs = std::filesystem;
  constexpr std::string_view ModuleOwner = "ReflectionSerializerIntegration";
  ActorRegistry& ActorClasses = ActorRegistry::GetInstance();
  FReflectionRegistry& Reflections = FReflectionRegistry::GetInstance();
  Check(
      ActorClasses.RegisterOwned<AReflectionSerializerTestActor>(std::string(ModuleOwner)),
      "Could not register the annotated serializer test actor."
  );
  Check(
      ReflectionGenerator::RegisterClass<AReflectionSerializerTestActor, AActor>(
          std::string(ModuleOwner)
      ) != 0,
      "Could not register reflected properties for the serializer test actor."
  );
  const FClass* ActorClass =
      Reflections.FindClass(AReflectionSerializerTestActor::StaticClassName());
  Check(ActorClass != nullptr, "Could not find reflected serializer test actor.");

  const fs::path TempDirectory =
      fs::temp_directory_path() /
      ("BroccoliReflectionTests-World-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(TempDirectory);
  const fs::path LevelPath = TempDirectory / "WorldRoundTrip.BLevel.json";
  {
    World SaveWorld;
    AReflectionSerializerTestActor* SourceActor =
        SaveWorld.SpawnActor<AReflectionSerializerTestActor>({}, FRotator(0.0F), true);
    Check(SourceActor != nullptr, "Could not spawn the annotated actor for saving.");
    SaveWorld.GetActorManager()->FlushPendingActors();
    Check(
        SaveWorld.GetActorManager()->AssignInstanceName(*SourceActor, "SerializedReflectionActor"),
        "Could not assign the test actor instance name."
    );

    Check(
        RequireProperty(*ActorClass, "Enabled").Set(SourceActor, true),
        "Could not set private bool."
    );
    Check(RequireProperty(*ActorClass, "Count").Set(SourceActor, 42), "Could not set private int.");
    Check(
        RequireProperty(*ActorClass, "Weight").Set(SourceActor, 0.625F),
        "Could not set private float."
    );
    Check(
        RequireProperty(*ActorClass, "Label").Set(SourceActor, std::string("round-trip")),
        "Could not set private string."
    );
    Check(
        RequireProperty(*ActorClass, "Offset").Set(SourceActor, FVector2D{8.0F, -9.0F}),
        "Could not set private Vector2D."
    );
    Check(
        RequireProperty(*ActorClass, "Position").Set(SourceActor, FVector3D{10.0F, 11.0F, -12.0F}),
        "Could not set private Vector3D."
    );
    Check(
        RequireProperty(*ActorClass, "AssetPath").Set(SourceActor, FPath("Textures/A.png")),
        "Could not set private Path."
    );
    Check(SourceActor->SetActorLocation3D({13.0F, 14.0F, 15.0F}), "Could not set actor location.");
    Check(
        SourceActor->SetActorRotation3D({0.0F, 0.3826834F, 0.0F, 0.9238795F}),
        "Could not set actor rotation."
    );
    Check(SourceActor->SetActorScale3D({2.0F, 3.0F, 4.0F}), "Could not set actor scale.");
    Check(
        LevelSerializer::Save(&SaveWorld, LevelPath.string(), ""),
        "LevelSerializer::Save failed for the annotated actor."
    );
    EditorClipboard Clipboard;
    Check(Clipboard.Copy(SourceActor), "Could not copy reflected actor.");
    World PasteWorld;
    AActor* Pasted = Clipboard.Paste(&PasteWorld, FVector3D{1.0F, 2.0F, 3.0F});
    Check(
        Pasted != nullptr && ReadProperty<FPath>(*ActorClass, "AssetPath", Pasted).String() ==
                                 "/Game/Textures/A.png",
        "Path did not round trip through EditorClipboard."
    );
  }

  FLevelMetaData LoadedMeta;
  {
    World LoadWorld;
    Check(
        LevelSerializer::Load(&LoadWorld, LevelPath.string(), false, &LoadedMeta),
        "LevelSerializer::Load failed for the annotated actor."
    );
    AReflectionSerializerTestActor* LoadedActor = FindSerializerTestActor(LoadWorld);
    Check(LoadedActor != nullptr, "LevelSerializer did not spawn the annotated actor.");
    Check(
        LoadedMeta.GameModeClassName.empty() &&
            LoadedActor->GetInstanceName() == "SerializedReflectionActor",
        "Loaded metadata or actor name did not round trip."
    );
    Check(
        ReadProperty<bool>(*ActorClass, "Enabled", LoadedActor) &&
            ReadProperty<int>(*ActorClass, "Count", LoadedActor) == 42 &&
            ReadProperty<float>(*ActorClass, "Weight", LoadedActor) == 0.625F &&
            ReadProperty<std::string>(*ActorClass, "Label", LoadedActor) == "round-trip",
        "Annotated scalar properties did not round trip through World and LevelSerializer."
    );
    Check(
        ReadProperty<FVector2D>(*ActorClass, "Offset", LoadedActor).X == 8.0F &&
            ReadProperty<FVector2D>(*ActorClass, "Offset", LoadedActor).Y == -9.0F &&
            ReadProperty<FVector3D>(*ActorClass, "Position", LoadedActor).X == 10.0F &&
            ReadProperty<FVector3D>(*ActorClass, "Position", LoadedActor).Y == 11.0F &&
            ReadProperty<FVector3D>(*ActorClass, "Position", LoadedActor).Z == -12.0F,
        "Annotated vector properties did not round trip through World and LevelSerializer."
    );
    Check(
        LoadedActor->CallbackCount == 1 && LoadedActor->LastOldCount == 7,
        "Loading a valid reflected value did not invoke its callback with the C++ initial value."
    );
    Check(
        ReadProperty<FPath>(*ActorClass, "AssetPath", LoadedActor).String() ==
                "/Game/Textures/A.png" &&
            LoadedActor->PathCallbackCount == 1 && LoadedActor->LastOldPath.Empty(),
        "Path did not round trip or invoke its callback."
    );
    const FTransform3D Transform = LoadedActor->GetActorTransform3D();
    Check(
        Transform.Location.X == 13.0F && Transform.Location.Y == 14.0F &&
            Transform.Location.Z == 15.0F && Transform.Scale.X == 2.0F &&
            Transform.Scale.Y == 3.0F && Transform.Scale.Z == 4.0F &&
            std::abs(Transform.Rotation.Y - 0.3826834F) < 0.0001F,
        "Actor transform did not round trip through World and LevelSerializer."
    );
  }

  {
    std::ifstream Input(LevelPath, std::ios::binary);
    Check(Input.is_open(), "Could not reopen the saved world level.");
    nlohmann::json Saved = nlohmann::json::parse(Input);
    Input.close();
    Check(
        Saved["actors"][0]["properties"]["Count"] == 42 && !Saved["actors"][0].contains("access"),
        "Private property changed the Level JSON schema."
    );
    Check(
        Saved["actors"][0]["properties"]["AssetPath"] == "/Game/Textures/A.png",
        "Path was not saved as a virtual path string."
    );
    Saved["actors"][0]["properties"]["Count"] = "invalid-int";
    Saved["actors"][0]["properties"]["AssetPath"] = "Textures/Legacy.png";
    WriteJson(LevelPath, Saved);
  }
  {
    World LoadWorldWithInvalidProperty;
    Check(
        LevelSerializer::Load(&LoadWorldWithInvalidProperty, LevelPath.string(), false),
        "LevelSerializer::Load failed when one reflected property had the wrong JSON type."
    );
    AReflectionSerializerTestActor* LoadedActor =
        FindSerializerTestActor(LoadWorldWithInvalidProperty);
    Check(LoadedActor != nullptr, "Invalid property prevented the actor from loading.");
    Check(
        ReadProperty<int>(*ActorClass, "Count", LoadedActor) == 7 &&
            LoadedActor->CallbackCount == 0,
        "Invalid property failed to preserve only its C++ initial value."
    );
    Check(
        ReadProperty<bool>(*ActorClass, "Enabled", LoadedActor) &&
            ReadProperty<float>(*ActorClass, "Weight", LoadedActor) == 0.625F &&
            ReadProperty<std::string>(*ActorClass, "Label", LoadedActor) == "round-trip" &&
            ReadProperty<FVector2D>(*ActorClass, "Offset", LoadedActor).X == 8.0F &&
            ReadProperty<FVector3D>(*ActorClass, "Position", LoadedActor).Z == -12.0F,
        "One invalid property prevented other reflected properties from loading."
    );
    Check(
        ReadProperty<FPath>(*ActorClass, "AssetPath", LoadedActor).String() ==
            "/Game/Textures/Legacy.png",
        "Legacy relative path was not normalized on load."
    );
  }

  const fs::path TempRoot = fs::weakly_canonical(fs::temp_directory_path());
  const fs::path CanonicalTempDirectory = fs::weakly_canonical(TempDirectory);
  Check(
      CanonicalTempDirectory.is_absolute() && CanonicalTempDirectory.parent_path() == TempRoot &&
          CanonicalTempDirectory.filename().string().starts_with("BroccoliReflectionTests-World-"),
      "Refusing to remove a world-test directory outside its dedicated temp root."
  );
  fs::remove_all(CanonicalTempDirectory);

  Reflections.UnregisterModule(ModuleOwner);
  ActorClasses.UnregisterModule(ModuleOwner);
}

void TestSpriteActorImagePathRoundTrip() {
  namespace fs = std::filesystem;
  ActorRegistry& Actors = ActorRegistry::GetInstance();
  FReflectionRegistry& Reflections = FReflectionRegistry::GetInstance();
  Check(Actors.Contains(ASprite2DActor::StaticClassName()), "2D Sprite Actor is not registered.");
  Check(Actors.Contains(ASprite3DActor::StaticClassName()), "3D Sprite Actor is not registered.");
  Check(!Actors.Contains("ASpriteActor"), "Legacy Sprite Actor is still registered.");

  const FClass* Class2D = Reflections.FindClass(ASprite2DActor::StaticClassName());
  const FClass* Class3D = Reflections.FindClass(ASprite3DActor::StaticClassName());
  Check(Class2D != nullptr && Class3D != nullptr, "Sprite Actor Reflection is missing.");
  const FProperty& Image2D = RequireProperty(*Class2D, "ImagePath");
  const FProperty& Image3D = RequireProperty(*Class3D, "ImagePath");
  Check(
      Image2D.Type == EPropertyType::Path && Image3D.Type == EPropertyType::Path &&
          Image2D.EditorMetadata.PathFilter == EPathFilter::Image &&
          Image3D.EditorMetadata.PathFilter == EPathFilter::Image,
      "Sprite ImagePath metadata is incorrect."
  );

  const fs::path TempDirectory =
      fs::temp_directory_path() /
      ("BroccoliReflectionTests-Sprites-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(TempDirectory);
  const fs::path LevelPath = TempDirectory / "Sprites.BLevel.json";
  {
    World SaveWorld;
    ASprite2DActor* Sprite2D = SaveWorld.SpawnActor<ASprite2DActor>({}, FRotator(0.0F), true);
    ASprite3DActor* Sprite3D = SaveWorld.SpawnActor<ASprite3DActor>({}, FRotator(0.0F), true);
    Check(Sprite2D != nullptr && Sprite3D != nullptr, "Could not spawn Sprite Actors.");
    SaveWorld.GetActorManager()->FlushPendingActors();
    Check(
        dynamic_cast<MSprite2DComponent*>(Sprite2D->GetRootComponent()) != nullptr,
        "2D Sprite Actor has the wrong root component."
    );
    Check(
        dynamic_cast<MSprite3DComponent*>(Sprite3D->GetRootComponent()) != nullptr,
        "3D Sprite Actor has the wrong root component."
    );
    Check(
        Image2D.Set(Sprite2D, FPath("Textures/A.png")) &&
            Image3D.Set(Sprite3D, FPath("Textures/B.png")),
        "Sprite ImagePath Reflection setter failed."
    );
    Check(
        Sprite2D->GetImagePath().String() == "/Game/Textures/A.png" &&
            Sprite3D->GetImagePath().String() == "/Game/Textures/B.png",
        "Sprite ImagePath getter did not return the reflected value."
    );
    Check(
        LevelSerializer::Save(&SaveWorld, LevelPath.string(), ""),
        "Could not save Sprite Actors to a level."
    );
    auto* Sprite3DRoot = dynamic_cast<MSprite3DComponent*>(Sprite3D->GetRootComponent());
    Sprite3DRoot->SetTexture(123);
    Check(
        Image3D.Set(Sprite3D, FPath()) && Sprite3DRoot->GetTexture() == 0,
        "Empty ImagePath did not clear the 3D Sprite texture handle."
    );
  }

  {
    std::ifstream Input(LevelPath, std::ios::binary);
    Check(Input.is_open(), "Could not read the Sprite Actor level.");
    const nlohmann::json Saved = nlohmann::json::parse(Input);
    Check(Saved.at("actors").size() == 2, "Sprite Actor count was not saved.");
    for (const auto& Entry : Saved.at("actors")) {
      const std::string ClassName = Entry.at("class").get<std::string>();
      if (ClassName == ASprite2DActor::StaticClassName()) {
        Check(
            Entry.at("properties").at("ImagePath") == "/Game/Textures/A.png",
            "2D Sprite ImagePath was not saved as a virtual path."
        );
      } else if (ClassName == ASprite3DActor::StaticClassName()) {
        Check(
            Entry.at("properties").at("ImagePath") == "/Game/Textures/B.png",
            "3D Sprite ImagePath was not saved as a virtual path."
        );
      } else {
        Check(false, "Unexpected Sprite Actor class in the saved level.");
      }
    }
  }

  {
    World LoadWorld;
    Check(
        LevelSerializer::Load(&LoadWorld, LevelPath.string(), false),
        "Could not reload the Sprite Actor level."
    );
    bool Found2D = false;
    bool Found3D = false;
    for (const std::unique_ptr<AActor>& Actor : LoadWorld.GetActorManager()->GetAllActors()) {
      if (auto* Sprite2D = dynamic_cast<ASprite2DActor*>(Actor.get())) {
        Found2D = Sprite2D->GetImagePath().String() == "/Game/Textures/A.png";
      } else if (auto* Sprite3D = dynamic_cast<ASprite3DActor*>(Actor.get())) {
        Found3D = Sprite3D->GetImagePath().String() == "/Game/Textures/B.png";
      }
    }
    Check(Found2D && Found3D, "Sprite Actor ImagePath did not round trip through the level.");
  }

  const fs::path TempRoot = fs::weakly_canonical(fs::temp_directory_path());
  const fs::path CanonicalTempDirectory = fs::weakly_canonical(TempDirectory);
  Check(
      CanonicalTempDirectory.is_absolute() && CanonicalTempDirectory.parent_path() == TempRoot &&
          CanonicalTempDirectory.filename().string().starts_with(
              "BroccoliReflectionTests-Sprites-"
          ),
      "Refusing to remove a Sprite test directory outside its dedicated temp root."
  );
  fs::remove_all(CanonicalTempDirectory);
}

void TestMaterialAndLightRoundTrip() {
  namespace fs = std::filesystem;
  const FClass* MeshClass =
      FReflectionRegistry::GetInstance().FindClass(AStaticMeshActor::StaticClassName());
  const FClass* LightClass =
      FReflectionRegistry::GetInstance().FindClass(ADirectionalLightActor::StaticClassName());
  Check(MeshClass != nullptr && LightClass != nullptr, "3D actor Reflection is missing.");
  Check(
      RequireProperty(*MeshClass, "ShadingModel").EditorMetadata.EnumOptions.size() == 2,
      "Shading model options are missing."
  );
  Check(
      RequireProperty(*MeshClass, "BaseColor").Type == EPropertyType::Color &&
          RequireProperty(*MeshClass, "BaseColorTexturePath").EditorMetadata.PathFilter ==
              EPathFilter::Image,
      "Material editor metadata is incorrect."
  );

  const fs::path TempDirectory =
      fs::temp_directory_path() /
      ("BroccoliReflectionTests-Material-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(TempDirectory);
  const fs::path LevelPath = TempDirectory / "Material.BLevel.json";
  {
    World SaveWorld;
    auto* Mesh = SaveWorld.SpawnActor<AStaticMeshActor>({}, FRotator(0.0F), true);
    auto* Light = SaveWorld.SpawnActor<ADirectionalLightActor>({}, FRotator(0.0F), true);
    Check(Mesh != nullptr && Light != nullptr, "Could not spawn material test actors.");
    SaveWorld.GetActorManager()->FlushPendingActors();
    Check(RequireProperty(*MeshClass, "ShadingModel").Set(Mesh, 0), "Could not set Unlit.");
    Check(
        RequireProperty(*MeshClass, "BaseColor").Set(Mesh, FColor{25, 50, 75, 255}),
        "Could not set base color."
    );
    Check(
        RequireProperty(*LightClass, "Intensity").Set(Light, 2.5f) &&
            RequireProperty(*LightClass, "Enabled").Set(Light, false),
        "Could not set directional light."
    );
    auto* MeshComponent = dynamic_cast<MStaticMeshComponent*>(Mesh->GetRootComponent());
    Check(MeshComponent != nullptr, "StaticMeshActor has the wrong root component.");
    const FMaterial3DDesc* Material =
        ResourceManager::GetInstance().GetMaterial3D(MeshComponent->GetMaterial());
    Check(
        Material != nullptr && Material->ShadingModel == EShadingModel3D::Unlit &&
            Material->BaseColor == FColor{25, 50, 75, 255},
        "Reflected values did not rebuild the runtime material."
    );
    Check(LevelSerializer::Save(&SaveWorld, LevelPath.string(), ""), "Could not save level.");
  }
  {
    World LoadWorld;
    Check(LevelSerializer::Load(&LoadWorld, LevelPath.string(), false), "Could not load level.");
    bool MeshRestored = false;
    bool LightRestored = false;
    for (const auto& Actor : LoadWorld.GetActorManager()->GetAllActors()) {
      if (auto* Mesh = dynamic_cast<AStaticMeshActor*>(Actor.get())) {
        auto* Component = dynamic_cast<MStaticMeshComponent*>(Mesh->GetRootComponent());
        const FMaterial3DDesc* Material = ResourceManager::GetInstance().GetMaterial3D(
            Component != nullptr ? Component->GetMaterial() : 0
        );
        MeshRestored = Material != nullptr && Material->ShadingModel == EShadingModel3D::Unlit &&
                       Material->BaseColor == FColor{25, 50, 75, 255};
      } else if (auto* Light = dynamic_cast<ADirectionalLightActor*>(Actor.get())) {
        auto* Component = dynamic_cast<MDirectionalLightComponent*>(Light->GetRootComponent());
        LightRestored =
            Component != nullptr && Component->GetIntensity() == 2.5f && !Component->IsEnabled();
      }
    }
    Check(MeshRestored && LightRestored, "Material or light did not round trip.");
  }
  const fs::path Canonical = fs::weakly_canonical(TempDirectory);
  Check(
      Canonical.parent_path() == fs::weakly_canonical(fs::temp_directory_path()) &&
          Canonical.filename().string().starts_with("BroccoliReflectionTests-Material-"),
      "Refusing to remove a directory outside the material test root."
  );
  fs::remove_all(Canonical);
}

void TestPluginLoadUnloadAndLiveActorDelay() {
  namespace fs = std::filesystem;
  PluginHost& Host = PluginHost::GetInstance();
  ActorRegistry& Actors = ActorRegistry::GetInstance();
  const fs::path PluginsDirectory = fs::current_path() / "Bin/x64/Debug/Plugins";
  Check(
      fs::exists(PluginsDirectory / "ExamplePlugin/plugin.json"),
      "Built ExamplePlugin manifest is missing."
  );
  Check(Host.Initialize(PluginsDirectory), "PluginHost failed to discover ExamplePlugin.");
  Check(
      Host.GetPluginCount() == 1 && Host.GetActivePluginCount() == 1,
      "ExamplePlugin did not load and activate."
  );
  Host.Update(0.016F);

  constexpr std::string_view ModuleOwner = "Plugin:ExamplePlugin";
  Check(
      Actors.RegisterOwned<AReflectionPluginTestActor>(std::string(ModuleOwner)),
      "Could not register the test actor under ExamplePlugin's module owner."
  );
  AReflectionPluginTestActor LiveActor;
  Actors.NotifySpawned(&LiveActor, AReflectionPluginTestActor::StaticClassName());
  Check(Actors.HasLiveActors(ModuleOwner), "ActorRegistry did not track the live test actor.");

  Host.Shutdown();
  Check(
      Host.GetPluginCount() == 1 && Host.GetActivePluginCount() == 1,
      "PluginHost unloaded a plugin while its module still had a live actor."
  );
  Check(Actors.HasLiveActors(ModuleOwner), "Live actor tracking was lost during delayed unload.");

  Actors.NotifyDestroyed(&LiveActor);
  Host.Shutdown();
  Check(
      Host.GetPluginCount() == 0 && Host.GetActivePluginCount() == 0,
      "PluginHost did not complete unload after the actor was destroyed."
  );
  Check(
      !Actors.Contains(AReflectionPluginTestActor::StaticClassName()),
      "Plugin unload left the module's actor class registered."
  );
}

void TestPluginHostExitWithLiveActor() {
  namespace fs = std::filesystem;
  PluginHost& Host = PluginHost::GetInstance();
  ActorRegistry& Actors = ActorRegistry::GetInstance();
  const fs::path PluginsDirectory = fs::current_path() / "Bin/x64/Debug/Plugins";
  Check(Host.Initialize(PluginsDirectory), "PluginHost failed to discover ExamplePlugin.");
  constexpr std::string_view ModuleOwner = "Plugin:ExamplePlugin";
  Check(
      Actors.RegisterOwned<AReflectionPluginTestActor>(std::string(ModuleOwner)),
      "Could not register the test actor under ExamplePlugin's module owner."
  );
  AReflectionPluginTestActor* LiveActor = new AReflectionPluginTestActor();
  Actors.NotifySpawned(LiveActor, AReflectionPluginTestActor::StaticClassName());
  Host.Shutdown();
  Check(
      Host.GetPluginCount() == 1 && Actors.HasLiveActors(ModuleOwner),
      "PluginHost did not retain a live-actor plugin at process exit."
  );
}
}  // namespace

int main(int ArgCount, char** Arguments) {
  int ExitCode = 0;
  try {
    if (ArgCount == 2 && std::string_view(Arguments[1]) == "--live-actor-exit") {
      TestPluginHostExitWithLiveActor();
    } else {
      TestNonPublicProperties();
      TestStaticActorReflectionRegistration();
      TestPathValueAndReflection();
      TestSixPropertyTypesAndMetadata();
      TestTypeMismatchAndCallbackFailure();
      TestInheritanceAndRegistry();
      TestLevelSerializerRoundTripAndLegacyVersions();
      TestWorldLevelSerializerReflectionRoundTrip();
      TestSpriteActorImagePathRoundTrip();
      TestMaterialAndLightRoundTrip();
      TestPluginLoadUnloadAndLiveActorDelay();
    }
  } catch (const std::exception& Error) {
    std::cerr << "Reflection tests failed: " << Error.what() << '\n';
    ExitCode = 1;
  }
  MLog::Shutdown();
  if (ExitCode != 0) return ExitCode;
  std::cout << "Reflection tests passed.\n";
  return 0;
}
