#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct FCreateNewActorRequest {
  std::string ClassName;
  std::string ParentClassName;
  std::filesystem::path OutputDirectory;
};

struct FCreateNewActorResult {
  bool bSuccess = false;
  std::string Message;
  std::filesystem::path HeaderPath;
  std::filesystem::path SourcePath;
};

class ActorClassGenerator {
 public:
  FCreateNewActorResult Generate(const FCreateNewActorRequest& Request);

 private:
  bool BuildActorHeaderIndex(const std::filesystem::path& ProjectRoot, std::string& ErrorMessage);

  bool bActorHeaderIndexBuilt = false;
  std::filesystem::path IndexedProjectRoot;
  std::unordered_map<std::string, std::string> ActorHeaderIndex;
  std::unordered_set<std::string> AmbiguousActorClasses;
};
