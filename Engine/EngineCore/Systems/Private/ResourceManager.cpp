#include "ResourceManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BroccoliRaylib.h"
#include "Log.h"
#include "RaylibResourceBridge.h"

namespace {
constexpr int InvalidResourceHandle = 0;

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
  bool OwnsFont = false;
};

class FRaylibResourceStore {
 public:
  int LoadTextureResource(const std::string& Path) {
    const auto Cached = TexturePathMap.find(Path);
    if (Cached != TexturePathMap.end()) return Cached->second;

    const Texture2D Texture = LoadTexture(Path.c_str());
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

  int GetFontResource(int PixelSize, int Thickness) {
    const std::string Key = std::to_string(PixelSize) + "_" + std::to_string(Thickness);
    const auto Cached = FontKeyMap.find(Key);
    if (Cached != FontKeyMap.end()) return Cached->second;

    FFontResource Resource;
    Resource.PixelSize = (std::max)(1, PixelSize);
    Resource.Path = FindJapaneseFontPath();
    AddAsciiCodepoints(Resource.Codepoints);
    if (!Resource.Path.empty()) {
      Resource.FontData = LoadFont(Resource);
      Resource.OwnsFont = IsFontValid(Resource.FontData);
    }
    if (!Resource.OwnsFont) {
      Resource.FontData = GetFontDefault();
      M_LOG("Japanese font file was not found. Falling back to the raylib default font.");
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
  static void AddAsciiCodepoints(std::unordered_set<int>& Codepoints) {
    for (int Codepoint = 0x20; Codepoint <= 0x7E; ++Codepoint) {
      Codepoints.insert(Codepoint);
    }
  }

  static std::string FindJapaneseFontPath() {
    static constexpr std::array<const char*, 6> Candidates = {
        "Resources/.Engine/NotoSansJP-Regular.ttf",
        "Resources/NotoSansJP-Regular.ttf",
        "C:/Windows/Fonts/meiryo.ttc",
        "C:/Windows/Fonts/YuGothM.ttc",
        "C:/Windows/Fonts/msgothic.ttc",
        "C:/Windows/Fonts/meiryo.ttf",
    };
    for (const char* Candidate : Candidates) {
      if (FileExists(Candidate)) return Candidate;
    }
    return {};
  }

  static Font LoadFont(const FFontResource& Resource) {
    std::vector<int> Codepoints(Resource.Codepoints.begin(), Resource.Codepoints.end());
    std::sort(Codepoints.begin(), Codepoints.end());
    return LoadFontEx(
        Resource.Path.c_str(),
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
  static constexpr std::array<const char*, 3> DefaultTextureCandidates = {
      "Resources/texture_Checker_64px.png",
      "Engine/Resources/texture_Checker_64px.png",
      "Engine/EngineSide/Files/texture_Checker_64px.png",
  };
  for (const char* Candidate : DefaultTextureCandidates) {
    ImplPtr->DefaultGraph = GetResourceStore().LoadTextureResource(Candidate);
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
  int Handle = GetResourceStore().LoadTextureResource(Path);
  if (Handle == InvalidResourceHandle) {
    M_LOG("Texture load failed: {}. Using the default texture.", Path);
    Handle = ImplPtr->DefaultGraph;
    GetResourceStore().AddTextureAlias(Path, Handle);
  }
  return Handle;
}

int ResourceManager::GetFont(int Size, int Thickness) {
  return GetResourceStore().GetFontResource(Size, Thickness);
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
