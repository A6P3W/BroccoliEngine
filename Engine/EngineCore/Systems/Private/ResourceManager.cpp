#include "ResourceManager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BroccoliRaylib.h"
#include "FileUtils.h"
#include "Log.h"
#include "PathResolver.h"
#include "RaylibResourceBridge.h"

namespace {
constexpr int InvalidResourceHandle = 0;

std::string ToLowerExtension(const std::filesystem::path& PathObj) {
  std::string Ext = FileUtils::PathToUtf8(PathObj.extension());
  std::transform(Ext.begin(), Ext.end(), Ext.begin(), [](unsigned char Character) {
    return static_cast<char>(std::tolower(Character));
  });
  return Ext;
}

bool ReadFileToMemory(const std::filesystem::path& Path, std::vector<unsigned char>& OutBuffer) {
  std::error_code ErrorCode;
  if (!std::filesystem::exists(Path, ErrorCode)) return false;

  std::ifstream File(Path, std::ios::binary | std::ios::ate);
  if (!File.is_open()) return false;

  const std::streamsize Size = File.tellg();
  if (Size <= 0 || Size > std::numeric_limits<int>::max()) return false;

  OutBuffer.resize(static_cast<size_t>(Size));
  File.seekg(0, std::ios::beg);
  return File.read(reinterpret_cast<char*>(OutBuffer.data()), Size).good();
}

Image LoadImageUtf8(const std::string& Path) {
  const std::filesystem::path PathObj = FileUtils::Utf8ToPath(Path);
  std::vector<unsigned char> Buffer;
  if (!ReadFileToMemory(PathObj, Buffer)) return Image{};

  const std::string Ext = ToLowerExtension(PathObj);
  return LoadImageFromMemory(Ext.c_str(), Buffer.data(), static_cast<int>(Buffer.size()));
}

struct FTextureResource {
  Texture2D Texture{};
  RenderTexture2D RenderTexture{};
  bool IsRenderTexture = false;
};

struct FFontResource {
  Font FontData{};
  std::string Path;
  std::unordered_set<int> Codepoints;
  int PixelSize = 16;
  int Weight = ResourceManager::DefaultFontWeight;
  bool OwnsFont = false;
};

// raylib 6.0 UnloadModel does not own material textures or runtime pose arrays.
void UnloadSkeletalModel(Model ModelData) {
  std::unordered_set<unsigned int> TextureIds;
  for (int Index = 0; Index < ModelData.materialCount; ++Index) {
    const auto* Maps = ModelData.materials[Index].maps;
    if (Maps == nullptr) continue;
    for (int MapIndex = 0; MapIndex <= MATERIAL_MAP_BRDF; ++MapIndex) {
      const Texture2D Texture = Maps[MapIndex].texture;
      if (Texture.id != 0 && Texture.id != rlGetTextureIdDefault() &&
          TextureIds.insert(Texture.id).second)
        UnloadTexture(Texture);
    }
  }
  MemFree(ModelData.currentPose);
  MemFree(ModelData.boneMatrices);
  UnloadModel(ModelData);
}

struct FAnimationClipSet {
  ModelAnimation* Animations = nullptr;
  int Count = 0;
  ~FAnimationClipSet() {
    if (Animations != nullptr) UnloadModelAnimations(Animations, Count);
  }
};

struct FModelResource {
  Model ModelData{};
  FTransform3D ImportTransform;
  FBox3D Bounds;
  std::string Path;
  std::shared_ptr<FAnimationClipSet> Clips;
  std::vector<int> ClipIndices;
  std::vector<std::string> ClipNames;
};

class FRaylibResourceStore {
 public:
  int LoadModelResource(const std::string& Path) {
    const auto Cached = ModelPathMap.find(Path);
    if (Cached != ModelPathMap.end()) return Cached->second;

    const std::string Extension = ToLowerExtension(FileUtils::Utf8ToPath(Path));
    if (Extension != ".glb" && Extension != ".gltf") {
      M_LOG(Log, "Model load rejected for unsupported extension: {}", Path);
      return InvalidResourceHandle;
    }

    Model ModelData = LoadModel(Path.c_str());
    if (!IsModelValid(ModelData)) {
      M_LOG(Log, "Model load failed: {}", Path);
      return InvalidResourceHandle;
    }

    const BoundingBox Bounds = GetModelBoundingBox(ModelData);
    const int Handle = NextHandle++;
    Models.emplace(
        Handle,
        FModelResource{
            ModelData,
            {},
            {{Bounds.min.x, Bounds.min.y, Bounds.min.z},
             {Bounds.max.x, Bounds.max.y, Bounds.max.z}},
            Path,
        }
    );
    ModelPathMap.emplace(Path, Handle);
    return Handle;
  }

  int CreateSkeletalModel(const std::string& Path) {
    const std::string Extension = ToLowerExtension(FileUtils::Utf8ToPath(Path));
    std::error_code Error;
    if ((Extension != ".glb" && Extension != ".gltf") ||
        !std::filesystem::is_regular_file(FileUtils::Utf8ToPath(Path), Error)) {
      M_LOG(Warning, "Skeletal model path missing or unsupported: {}", Path);
      return 0;
    }
    Model ModelData = LoadModel(Path.c_str());
    if (!IsModelValid(ModelData) || ModelData.skeleton.boneCount <= 0 ||
        ModelData.skeleton.bones == nullptr || ModelData.skeleton.bindPose == nullptr ||
        ModelData.currentPose == nullptr || ModelData.boneMatrices == nullptr) {
      M_LOG(Warning, "Skeletal model load failed or skin missing: {}", Path);
      UnloadSkeletalModel(ModelData);
      return 0;
    }
    auto Clips = AnimationPathMap[Path].lock();
    if (!Clips) {
      Clips = std::make_shared<FAnimationClipSet>();
      Clips->Animations = LoadModelAnimations(Path.c_str(), &Clips->Count);
      if (Clips->Animations == nullptr) Clips->Count = 0;
      AnimationPathMap[Path] = Clips;
    }
    const BoundingBox Bounds = GetModelBoundingBox(ModelData);
    FModelResource Resource;
    Resource.ModelData = ModelData;
    Resource.Bounds = {
        {Bounds.min.x, Bounds.min.y, Bounds.min.z}, {Bounds.max.x, Bounds.max.y, Bounds.max.z}
    };
    Resource.Path = Path;
    Resource.Clips = Clips;
    std::unordered_set<std::string> Names;
    for (int Index = 0; Index < Clips->Count; ++Index) {
      const auto& Clip = Clips->Animations[Index];
      if (Clip.keyframeCount <= 0 || Clip.keyframePoses == nullptr ||
          !IsModelAnimationValid(ModelData, Clip)) {
        M_LOG(Warning, "Invalid skeletal clip {} in {}", Index, Path);
        continue;
      }
      bool Valid = true;
      for (int Frame = 0; Frame < Clip.keyframeCount; ++Frame) {
        if (Clip.keyframePoses[Frame] == nullptr) Valid = false;
      }
      if (!Valid) {
        M_LOG(Warning, "Missing skeletal clip poses {} in {}", Index, Path);
        continue;
      }
      const auto NameEnd = std::find(std::begin(Clip.name), std::end(Clip.name), '\0');
      std::string Base(Clip.name, NameEnd);
      if (Base.empty()) Base = "Animation_" + std::to_string(Index);
      std::string Name = Base;
      int Suffix = 1;
      while (!Names.insert(Name).second) Name = Base + "_" + std::to_string(Suffix++);
      Resource.ClipIndices.push_back(Index);
      Resource.ClipNames.push_back(std::move(Name));
    }
    const int Handle = NextHandle++;
    Models.emplace(Handle, std::move(Resource));
    return Handle;
  }

  void DestroySkeletalModel(int Handle) {
    const auto It = Models.find(Handle);
    if (It == Models.end() || !It->second.Clips) return;
    const std::string Path = It->second.Path;
    UnloadSkeletalModel(It->second.ModelData);
    Models.erase(It);
    const auto Cache = AnimationPathMap.find(Path);
    if (Cache != AnimationPathMap.end() && Cache->second.expired()) AnimationPathMap.erase(Cache);
  }

  const FModelResource* FindModel(int Handle) const {
    const auto It = Models.find(Handle);
    return It == Models.end() ? nullptr : &It->second;
  }

  int LoadTextureResource(const std::string& Path) {
    const auto Cached = TexturePathMap.find(Path);
    if (Cached != TexturePathMap.end()) return Cached->second;

    Image Img = LoadImageUtf8(Path);
    if (!IsImageValid(Img)) return InvalidResourceHandle;

    Texture2D Texture = LoadTextureFromImage(Img);
    UnloadImage(Img);
    if (!IsTextureValid(Texture)) return InvalidResourceHandle;

    const int Handle = NextHandle++;
    Textures.emplace(Handle, FTextureResource{Texture, {}, false});
    TexturePathMap.emplace(Path, Handle);
    return Handle;
  }

  int CreateDefaultTexture() {
    const Image ImageData = GenImageChecked(64, 64, 8, 8, MAGENTA, BLACK);
    const Texture2D Texture = LoadTextureFromImage(ImageData);
    UnloadImage(ImageData);
    if (!IsTextureValid(Texture)) return InvalidResourceHandle;

    const int Handle = NextHandle++;
    Textures.emplace(Handle, FTextureResource{Texture, {}, false});
    TexturePathMap.emplace("__broccoli_default_texture__", Handle);
    return Handle;
  }

  void AddTextureAlias(const std::string& Path, int Handle) {
    TexturePathMap.insert_or_assign(Path, Handle);
  }

  int CreateRenderTextureResource(int Width, int Height) {
    const RenderTexture2D RenderTexture = LoadRenderTexture(Width, Height);
    if (!IsRenderTextureValid(RenderTexture)) return InvalidResourceHandle;

    SetTextureFilter(RenderTexture.texture, TEXTURE_FILTER_BILINEAR);
    const int Handle = NextHandle++;
    Textures.emplace(Handle, FTextureResource{RenderTexture.texture, RenderTexture, true});
    return Handle;
  }

  const FTextureResource* FindTexture(int Handle) const {
    const auto It = Textures.find(Handle);
    return It == Textures.end() ? nullptr : &It->second;
  }

  FTextureResource* FindTexture(int Handle) {
    const auto It = Textures.find(Handle);
    return It == Textures.end() ? nullptr : &It->second;
  }

  void ReleaseTexture(int Handle) {
    const auto It = Textures.find(Handle);
    if (It == Textures.end()) return;

    if (It->second.IsRenderTexture) {
      UnloadRenderTexture(It->second.RenderTexture);
    } else {
      UnloadTexture(It->second.Texture);
    }
    Textures.erase(It);
    for (auto PathIt = TexturePathMap.begin(); PathIt != TexturePathMap.end();) {
      if (PathIt->second == Handle) {
        PathIt = TexturePathMap.erase(PathIt);
      } else {
        ++PathIt;
      }
    }
  }

  int GetFontResource(int PixelSize, int Weight) {
    Weight = ResourceManager::NormalizeFontWeight(Weight);
    const int NormalizedPixelSize = (std::max)(1, PixelSize);
    const std::string Key = std::to_string(NormalizedPixelSize) + "_" + std::to_string(Weight);
    const auto Cached = FontKeyMap.find(Key);
    if (Cached != FontKeyMap.end()) return Cached->second;

    FFontResource Resource;
    Resource.PixelSize = NormalizedPixelSize;
    Resource.Weight = Weight;

    Resource.Path = GetFontPath(Weight);
    if (!FontFileExists(Resource.Path)) {
      M_LOG(
          Log,
          "Font file for weight {} was not found at: {}. Falling back to weight {}.",
          Weight,
          Resource.Path,
          ResourceManager::DefaultFontWeight
      );
      Resource.Path = GetFontPath(ResourceManager::DefaultFontWeight);
    }

    AddAsciiCodepoints(Resource.Codepoints);
    Resource.FontData = LoadFont(Resource);
    Resource.OwnsFont = IsFontValid(Resource.FontData);
    if (!Resource.OwnsFont) {
      Resource.FontData = GetFontDefault();
      M_LOG(
          Log,
          "Font file was not found at: {}. Falling back to the raylib default font.",
          Resource.Path
      );
    }

    const int Handle = NextHandle++;
    Fonts.emplace(Handle, std::move(Resource));
    FontKeyMap.emplace(Key, Handle);
    return Handle;
  }

  FFontResource* FindFont(int Handle) {
    const auto It = Fonts.find(Handle);
    return It == Fonts.end() ? nullptr : &It->second;
  }

  const FFontResource* FindFont(int Handle) const {
    const auto It = Fonts.find(Handle);
    return It == Fonts.end() ? nullptr : &It->second;
  }

  void EnsureFontCodepoints(int Handle, const std::string& Text) {
    FFontResource* Resource = FindFont(Handle);
    if (Resource == nullptr || Resource->Path.empty() || Text.empty()) return;

    int CodepointCount = 0;
    int* LoadedCodepoints = LoadCodepoints(Text.c_str(), &CodepointCount);
    bool Changed = false;
    for (int Index = 0; Index < CodepointCount; ++Index) {
      Changed |= Resource->Codepoints.insert(LoadedCodepoints[Index]).second;
    }
    UnloadCodepoints(LoadedCodepoints);
    if (!Changed) return;

    Font NewFont = LoadFont(*Resource);
    if (!IsFontValid(NewFont)) return;

    if (Resource->OwnsFont) UnloadFont(Resource->FontData);
    Resource->FontData = NewFont;
    Resource->OwnsFont = true;
  }

  void ReleaseAll() {
    for (auto& [Handle, Resource] : Fonts) {
      if (Resource.OwnsFont) UnloadFont(Resource.FontData);
    }
    Fonts.clear();
    FontKeyMap.clear();

    for (auto& [Handle, Resource] : Textures) {
      if (Resource.IsRenderTexture) {
        UnloadRenderTexture(Resource.RenderTexture);
      } else {
        UnloadTexture(Resource.Texture);
      }
    }
    Textures.clear();
    TexturePathMap.clear();

    for (auto& [Handle, Resource] : Models) {
      if (Resource.Clips)
        UnloadSkeletalModel(Resource.ModelData);
      else
        UnloadModel(Resource.ModelData);
    }
    Models.clear();
    ModelPathMap.clear();
    AnimationPathMap.clear();
  }

 private:
  static std::string GetFontPath(int Weight) {
    static constexpr std::array<const char*, 9> FontFileNames = {
        "NotoSansJP-Thin.ttf",
        "NotoSansJP-ExtraLight.ttf",
        "NotoSansJP-Light.ttf",
        "NotoSansJP-Regular.ttf",
        "NotoSansJP-Medium.ttf",
        "NotoSansJP-SemiBold.ttf",
        "NotoSansJP-Bold.ttf",
        "NotoSansJP-ExtraBold.ttf",
        "NotoSansJP-Black.ttf",
    };
    const size_t Index = static_cast<size_t>(Weight / ResourceManager::FontWeightStep - 1);
    return PathResolver::Resolve("/Engine/Fonts/Noto_Sans_JP/" + std::string(FontFileNames[Index]));
  }

  static bool FontFileExists(const std::string& Path) {
    std::error_code ErrorCode;
    return std::filesystem::exists(FileUtils::Utf8ToPath(Path), ErrorCode);
  }

  static void AddAsciiCodepoints(std::unordered_set<int>& Codepoints) {
    for (int Codepoint = 0x20; Codepoint <= 0x7E; ++Codepoint) {
      Codepoints.insert(Codepoint);
    }
  }

  static Font LoadFont(const FFontResource& Resource) {
    std::vector<int> Codepoints(Resource.Codepoints.begin(), Resource.Codepoints.end());
    std::sort(Codepoints.begin(), Codepoints.end());

    const std::filesystem::path PathObj = FileUtils::Utf8ToPath(Resource.Path);
    std::vector<unsigned char> Buffer;
    if (!ReadFileToMemory(PathObj, Buffer)) return Font{};

    const std::string Ext = ToLowerExtension(PathObj);
    return LoadFontFromMemory(
        Ext.c_str(),
        Buffer.data(),
        static_cast<int>(Buffer.size()),
        Resource.PixelSize,
        Codepoints.data(),
        static_cast<int>(Codepoints.size())
    );
  }

  std::unordered_map<std::string, std::weak_ptr<FAnimationClipSet>> AnimationPathMap;
  int NextHandle = 1;
  std::unordered_map<int, FTextureResource> Textures;
  std::unordered_map<std::string, int> TexturePathMap;
  std::unordered_map<int, FFontResource> Fonts;
  std::unordered_map<std::string, int> FontKeyMap;
  std::unordered_map<int, FModelResource> Models;
  std::unordered_map<std::string, int> ModelPathMap;
};

FRaylibResourceStore& GetResourceStore() {
  static FRaylibResourceStore Store;
  return Store;
}
}  // namespace

struct ResourceManager::Impl {
  int DefaultGraph = InvalidResourceHandle;
};

ResourceManager::ResourceManager() : ImplPtr(new Impl()) {
  static constexpr std::array<const char*, 1> DefaultTextureCandidates = {
      "Engine/texture_Checker_64px.png",
  };
  for (const char* Candidate : DefaultTextureCandidates) {
    const std::string ResolvedPath = PathResolver::Resolve(Candidate);
    ImplPtr->DefaultGraph = GetResourceStore().LoadTextureResource(ResolvedPath);
    if (ImplPtr->DefaultGraph != InvalidResourceHandle) break;
  }
  if (ImplPtr->DefaultGraph == InvalidResourceHandle) {
    ImplPtr->DefaultGraph = GetResourceStore().CreateDefaultTexture();
  }
}

ResourceManager::~ResourceManager() {
  ReleaseResourceGraph();
  delete ImplPtr;
}

ResourceManager& ResourceManager::GetInstance() {
  static ResourceManager Instance;
  return Instance;
}

int ResourceManager::LoadResourceGraph(const FPath& Path) {
  const std::string ResolvedPath = PathResolver::Resolve(Path.String());
  int Handle = GetResourceStore().LoadTextureResource(ResolvedPath);
  if (Handle == InvalidResourceHandle) {
    M_LOG(Log, "Texture load failed: {}. Using the default texture.", ResolvedPath);
    Handle = ImplPtr->DefaultGraph;
    GetResourceStore().AddTextureAlias(ResolvedPath, Handle);
  }
  return Handle;
}

int ResourceManager::LoadResourceModel(const FPath& Path) {
  const std::string ResolvedPath = PathResolver::Resolve(Path.String());
  return GetResourceStore().LoadModelResource(ResolvedPath);
}

int ResourceManager::CreateSkeletalMeshInstance(const FPath& Path) {
  return GetResourceStore().CreateSkeletalModel(PathResolver::Resolve(Path.String()));
}

void ResourceManager::DestroySkeletalMeshInstance(int Handle) {
  GetResourceStore().DestroySkeletalModel(Handle);
}

std::vector<std::string> ResourceManager::GetSkeletalMeshClipNames(int Handle) const {
  const auto* Resource = GetResourceStore().FindModel(Handle);
  return Resource == nullptr ? std::vector<std::string>{} : Resource->ClipNames;
}

int ResourceManager::GetSkeletalMeshClipFrameCount(int Handle, int Index) const {
  const auto* Resource = GetResourceStore().FindModel(Handle);
  if (Resource == nullptr || !Resource->Clips || Index < 0 ||
      static_cast<size_t>(Index) >= Resource->ClipIndices.size())
    return 0;
  return Resource->Clips->Animations[Resource->ClipIndices[Index]].keyframeCount;
}

bool ResourceManager::ApplySkeletalMeshFrame(int Handle, int Index, float Frame) {
  const auto* Resource = GetResourceStore().FindModel(Handle);
  const int Count = GetSkeletalMeshClipFrameCount(Handle, Index);
  if (Resource == nullptr || Count <= 0 || !std::isfinite(Frame) || Frame < 0.0f) return false;
  UpdateModelAnimation(
      Resource->ModelData,
      Resource->Clips->Animations[Resource->ClipIndices[Index]],
      (std::min)(Frame, static_cast<float>(Count - 1))
  );
  return true;
}

bool ResourceManager::IsModelValid(int Handle) const {
  return GetResourceStore().FindModel(Handle) != nullptr;
}

bool ResourceManager::GetModelBounds(int Handle, FBox3D& OutBounds) const {
  const FModelResource* Resource = GetResourceStore().FindModel(Handle);
  if (Resource == nullptr) return false;
  OutBounds = Resource->Bounds;
  return true;
}

int ResourceManager::NormalizeFontWeight(int Weight) {
  if (Weight >= MinFontWeight && Weight <= MaxFontWeight && Weight % FontWeightStep == 0) {
    return Weight;
  }

  M_LOG(Log, "Invalid font weight: {}. Falling back to {}.", Weight, DefaultFontWeight);
  return DefaultFontWeight;
}

int ResourceManager::GetFont(int Size, int Weight) {
  return GetResourceStore().GetFontResource(Size, Weight);
}

int ResourceManager::GetTextWidth(const std::string& Text, int FontHandle) {
  return static_cast<int>(std::ceil(MeasureRaylibText(FontHandle, Text).x));
}

int ResourceManager::GetFontPixelSize(int FontHandle) const {
  return static_cast<int>(GetRaylibFontSize(FontHandle));
}

void ResourceManager::ReleaseResourceGraph() { ReleaseAllResources(); }

void ResourceManager::ReleaseAllResources() {
  GetResourceStore().ReleaseAll();
  ImplPtr->DefaultGraph = InvalidResourceHandle;
}

const Texture2D* GetRaylibTexture(int Handle) {
  const FTextureResource* Resource = GetResourceStore().FindTexture(Handle);
  return Resource == nullptr ? nullptr : &Resource->Texture;
}

Rectangle GetRaylibTextureSource(int Handle) {
  const FTextureResource* Resource = GetResourceStore().FindTexture(Handle);
  if (Resource == nullptr) return {};
  const float Height = Resource->IsRenderTexture ? -static_cast<float>(Resource->Texture.height)
                                                 : static_cast<float>(Resource->Texture.height);
  return {0.0f, 0.0f, static_cast<float>(Resource->Texture.width), Height};
}

bool IsRaylibRenderTexture(int Handle) {
  const FTextureResource* Resource = GetResourceStore().FindTexture(Handle);
  return Resource != nullptr && Resource->IsRenderTexture;
}

bool GetRaylibTextureSize(int Handle, int& OutWidth, int& OutHeight) {
  const FTextureResource* Resource = GetResourceStore().FindTexture(Handle);
  if (Resource == nullptr) return false;
  OutWidth = Resource->Texture.width;
  OutHeight = Resource->Texture.height;
  return OutWidth > 0 && OutHeight > 0;
}

const Model* GetRaylibModel(int Handle) {
  const FModelResource* Resource = GetResourceStore().FindModel(Handle);
  return Resource == nullptr ? nullptr : &Resource->ModelData;
}

const FTransform3D* GetRaylibModelImportTransform(int Handle) {
  const FModelResource* Resource = GetResourceStore().FindModel(Handle);
  return Resource == nullptr ? nullptr : &Resource->ImportTransform;
}

const Font* GetRaylibFont(int Handle, const std::string& Text) {
  GetResourceStore().EnsureFontCodepoints(Handle, Text);
  const FFontResource* Resource = GetResourceStore().FindFont(Handle);
  return Resource == nullptr ? nullptr : &Resource->FontData;
}

float GetRaylibFontSize(int Handle) {
  const FFontResource* Resource = GetResourceStore().FindFont(Handle);
  return Resource == nullptr ? 0.0f : static_cast<float>(Resource->PixelSize);
}

Vector2 MeasureRaylibText(int Handle, const std::string& Text) {
  const Font* FontData = GetRaylibFont(Handle, Text);
  const float FontSize = GetRaylibFontSize(Handle);
  if (FontData == nullptr || FontSize <= 0.0f || Text.empty()) return {};
  return MeasureTextEx(*FontData, Text.c_str(), FontSize, 0.0f);
}

int CreateRaylibRenderTexture(int Width, int Height) {
  return GetResourceStore().CreateRenderTextureResource(Width, Height);
}

void ReleaseRaylibTexture(int Handle) { GetResourceStore().ReleaseTexture(Handle); }

bool BeginRaylibRenderTexture(int Handle) {
  FTextureResource* Resource = GetResourceStore().FindTexture(Handle);
  if (Resource == nullptr || !Resource->IsRenderTexture) return false;
  BeginTextureMode(Resource->RenderTexture);
  return true;
}

void EndRaylibRenderTexture() { EndTextureMode(); }
