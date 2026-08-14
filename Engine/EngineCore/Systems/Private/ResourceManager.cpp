#include "ResourceManager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
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

class FRaylibResourceStore {
 public:
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

  int NextHandle = 1;
  std::unordered_map<int, FTextureResource> Textures;
  std::unordered_map<std::string, int> TexturePathMap;
  std::unordered_map<int, FFontResource> Fonts;
  std::unordered_map<std::string, int> FontKeyMap;
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

int ResourceManager::LoadResourceGraph(const std::string& Path) {
  const std::string ResolvedPath = PathResolver::Resolve(Path);
  int Handle = GetResourceStore().LoadTextureResource(ResolvedPath);
  if (Handle == InvalidResourceHandle) {
    M_LOG(Log, "Texture load failed: {}. Using the default texture.", ResolvedPath);
    Handle = ImplPtr->DefaultGraph;
    GetResourceStore().AddTextureAlias(ResolvedPath, Handle);
  }
  return Handle;
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

void ResourceManager::ReleaseResourceGraph() {
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
