#include "Reflection.h"

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "Log.h"

std::vector<const FProperty*> FClass::GetProperties() const {
  std::vector<const FProperty*> Properties;
  if (BaseClass != nullptr) Properties = BaseClass->GetProperties();
  for (const FProperty& Property : OwnProperties) Properties.push_back(&Property);
  return Properties;
}

const FProperty* FClass::FindProperty(std::string_view PropertyName) const {
  for (const FProperty* Property : GetProperties()) {
    if (Property->Name == PropertyName) return Property;
  }
  return nullptr;
}

struct FReflectionRegistry::FImpl {
  struct FEntry {
    FClass Class;
    std::string ModuleOwner;
    FToken Token;
  };
  std::unordered_map<std::string, std::unique_ptr<FEntry>> Classes;
  FToken NextToken = 1;
};

FReflectionRegistry::FReflectionRegistry() : Impl(new FImpl()) {}

FReflectionRegistry::~FReflectionRegistry() { delete Impl; }

FReflectionRegistry& FReflectionRegistry::GetInstance() {
  static FReflectionRegistry Instance;
  return Instance;
}

FReflectionRegistry::FToken FReflectionRegistry::Register(FClass Class, std::string ModuleOwner) {
  if (Class.Name.empty() || Impl->Classes.contains(Class.Name)) {
    M_LOG(Error, "Reflection class name is empty or already registered: {}", Class.Name);
    return 0;
  }
  if (Class.BaseClass != nullptr) {
    const bool BaseRegistered = std::ranges::any_of(Impl->Classes, [&Class](const auto& Entry) {
      return &Entry.second->Class == Class.BaseClass;
    });
    if (!BaseRegistered) {
      M_LOG(Error, "Reflection base class is not registered for '{}'.", Class.Name);
      return 0;
    }
  }
  const auto Properties = Class.GetProperties();
  for (const FProperty* Property : Properties) {
    if (Property->Name.empty() || Property->Get == nullptr || Property->Set == nullptr) {
      M_LOG(Error, "Reflection class '{}' has an invalid property.", Class.Name);
      return 0;
    }
    for (const FProperty* Other : Properties) {
      if (Other != Property && Other->Name == Property->Name) {
        M_LOG(
            Error, "Reflection class '{}' has duplicate property '{}'.", Class.Name, Property->Name
        );
        return 0;
      }
    }
  }
  const FToken Token = Impl->NextToken++;
  const std::string Name = Class.Name;
  Impl->Classes.emplace(
      Name, std::make_unique<FImpl::FEntry>(std::move(Class), std::move(ModuleOwner), Token)
  );
  return Token;
}

bool FReflectionRegistry::Unregister(FToken Token) {
  for (auto Iterator = Impl->Classes.begin(); Iterator != Impl->Classes.end(); ++Iterator) {
    if (Iterator->second->Token == Token) {
      const FClass* Target = &Iterator->second->Class;
      for (const auto& [Name, Entry] : Impl->Classes) {
        if (Entry->Class.BaseClass == Target) {
          M_LOG(
              Error,
              "Cannot unregister Reflection class '{}': '{}' derives from it.",
              Target->Name,
              Name
          );
          return false;
        }
      }
      Impl->Classes.erase(Iterator);
      return true;
    }
  }
  return false;
}

bool FReflectionRegistry::UnregisterModule(std::string_view ModuleOwner) {
  for (const auto& [Name, Entry] : Impl->Classes) {
    if (Entry->ModuleOwner == ModuleOwner) continue;
    for (const FClass* Base = Entry->Class.BaseClass; Base != nullptr; Base = Base->BaseClass) {
      const auto BaseEntry = Impl->Classes.find(Base->Name);
      if (BaseEntry != Impl->Classes.end() && BaseEntry->second->ModuleOwner == ModuleOwner) {
        M_LOG(
            Error, "Cannot unload Reflection module '{}': '{}' depends on it.", ModuleOwner, Name
        );
        return false;
      }
    }
  }
  std::erase_if(Impl->Classes, [ModuleOwner](const auto& Entry) {
    return Entry.second->ModuleOwner == ModuleOwner;
  });
  return true;
}

const FClass* FReflectionRegistry::FindClass(std::string_view ClassName) const {
  const auto Iterator = Impl->Classes.find(std::string(ClassName));
  return Iterator == Impl->Classes.end() ? nullptr : &Iterator->second->Class;
}

bool FReflectionRegistry::HasModule(std::string_view ModuleOwner) const {
  return std::ranges::any_of(Impl->Classes, [ModuleOwner](const auto& Entry) {
    return Entry.second->ModuleOwner == ModuleOwner;
  });
}

bool IsValidUnicodeScalarString(std::string_view Value, std::size_t MaxLength) {
  std::size_t Count = 0;
  for (std::size_t Index = 0; Index < Value.size();) {
    const unsigned char First = static_cast<unsigned char>(Value[Index]);
    int Length = 0;
    unsigned int CodePoint = 0;
    if (First <= 0x7F) {
      Length = 1;
      CodePoint = First;
    } else if (First >= 0xC2 && First <= 0xDF) {
      Length = 2;
      CodePoint = First & 0x1F;
    } else if (First >= 0xE0 && First <= 0xEF) {
      Length = 3;
      CodePoint = First & 0x0F;
    } else if (First >= 0xF0 && First <= 0xF4) {
      Length = 4;
      CodePoint = First & 0x07;
    } else {
      return false;
    }
    if (Index + Length > Value.size()) return false;
    for (int Offset = 1; Offset < Length; ++Offset) {
      const unsigned char Next = static_cast<unsigned char>(Value[Index + Offset]);
      if ((Next & 0xC0) != 0x80) return false;
      CodePoint = (CodePoint << 6) | (Next & 0x3F);
    }
    if ((Length == 2 && CodePoint < 0x80) || (Length == 3 && CodePoint < 0x800) ||
        (Length == 4 && CodePoint < 0x10000) || (CodePoint >= 0xD800 && CodePoint <= 0xDFFF) ||
        CodePoint > 0x10FFFF)
      return false;
    ++Count;
    if (Count > MaxLength) return false;
    Index += Length;
  }
  return true;
}
