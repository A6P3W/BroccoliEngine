#include "ActorClassGenerator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <system_error>
#include <unordered_set>
#include <utility>

#include "ActorRegistry.h"

namespace {
constexpr const char* TemplateRelativePath = "Engine/Editor/Templates/CreateNewActor";

FCreateNewActorResult Fail(std::string Message) {
  FCreateNewActorResult Result;
  Result.Message = std::move(Message);
  return Result;
}

std::filesystem::path FindProjectRoot() {
  std::error_code ErrorCode;
  std::filesystem::path Candidate = std::filesystem::current_path(ErrorCode);
  if (ErrorCode) {
    return {};
  }

  while (!Candidate.empty()) {
    ErrorCode.clear();
    const bool bHasProject =
        std::filesystem::exists(Candidate / "Engine/BroccoliEngine.vcxproj", ErrorCode);
    ErrorCode.clear();
    const bool bHasTemplates = std::filesystem::exists(Candidate / TemplateRelativePath, ErrorCode);
    if (bHasProject && bHasTemplates) {
      return Candidate;
    }

    const std::filesystem::path Parent = Candidate.parent_path();
    if (Parent == Candidate) {
      break;
    }
    Candidate = Parent;
  }
  return {};
}

bool IsCppIdentifier(const std::string& Value) {
  if (Value.empty()) {
    return false;
  }
  const auto IsIdentifierStart = [](unsigned char Character) {
    return std::isalpha(Character) != 0 || Character == '_';
  };
  const auto IsIdentifierCharacter = [](unsigned char Character) {
    return std::isalnum(Character) != 0 || Character == '_';
  };

  return IsIdentifierStart(static_cast<unsigned char>(Value.front())) &&
         std::all_of(Value.begin() + 1, Value.end(), [&](char Character) {
           return IsIdentifierCharacter(static_cast<unsigned char>(Character));
         });
}

bool IsCppKeyword(const std::string& Value) {
  static const std::unordered_set<std::string> Keywords = {
      "alignas",       "alignof",     "and",
      "and_eq",        "asm",         "auto",
      "bitand",        "bitor",       "bool",
      "break",         "case",        "catch",
      "char",          "char8_t",     "char16_t",
      "char32_t",      "class",       "compl",
      "concept",       "const",       "consteval",
      "constexpr",     "constinit",   "const_cast",
      "continue",      "co_await",    "co_return",
      "co_yield",      "decltype",    "default",
      "delete",        "do",          "double",
      "dynamic_cast",  "else",        "enum",
      "explicit",      "export",      "extern",
      "false",         "float",       "for",
      "friend",        "goto",        "if",
      "inline",        "int",         "long",
      "mutable",       "namespace",   "new",
      "noexcept",      "not",         "not_eq",
      "nullptr",       "operator",    "or",
      "or_eq",         "private",     "protected",
      "public",        "register",    "reinterpret_cast",
      "requires",      "return",      "short",
      "signed",        "sizeof",      "static",
      "static_assert", "static_cast", "struct",
      "switch",        "template",    "this",
      "thread_local",  "throw",       "true",
      "try",           "typedef",     "typeid",
      "typename",      "union",       "unsigned",
      "using",         "virtual",     "void",
      "volatile",      "wchar_t",     "while",
      "xor",           "xor_eq"
  };
  return Keywords.contains(Value);
}

bool ReadTextFile(const std::filesystem::path& Path, std::string& Content) {
  std::ifstream Stream(Path, std::ios::binary);
  if (!Stream) {
    return false;
  }
  Content.assign(std::istreambuf_iterator<char>(Stream), std::istreambuf_iterator<char>());
  return Stream.good() || Stream.eof();
}

bool WriteTextFile(const std::filesystem::path& Path, const std::string& Content) {
  std::ofstream Stream(Path, std::ios::binary | std::ios::trunc);
  if (!Stream) {
    return false;
  }
  Stream.write(Content.data(), static_cast<std::streamsize>(Content.size()));
  return Stream.good();
}

void ReplaceAll(std::string& Content, const std::string& Tag, const std::string& Value) {
  size_t Position = 0;
  while ((Position = Content.find(Tag, Position)) != std::string::npos) {
    Content.replace(Position, Tag.length(), Value);
    Position += Value.length();
  }
}

std::filesystem::path FindTemplate(
    const std::filesystem::path& TemplateDirectory,
    const std::string& ParentClassName,
    const char* Extension
) {
  std::error_code ErrorCode;
  const std::filesystem::path ParentTemplate =
      TemplateDirectory / (ParentClassName + Extension + ".template");
  if (std::filesystem::is_regular_file(ParentTemplate, ErrorCode)) {
    return ParentTemplate;
  }
  return TemplateDirectory / (std::string("Default") + Extension + ".template");
}

bool IsSearchableHeader(const std::filesystem::path& Path) {
  if (Path.extension() != ".h") {
    return false;
  }
  return std::none_of(Path.begin(), Path.end(), [](const std::filesystem::path& Part) {
    return Part == "ThirdParty" || Part == ".git" || Part == "vcpkg_installed";
  });
}
}  // namespace

bool ActorClassGenerator::BuildActorHeaderIndex(
    const std::filesystem::path& ProjectRoot, std::string& ErrorMessage
) {
  ActorHeaderIndex.clear();
  AmbiguousActorClasses.clear();

  std::unordered_map<std::string, std::unordered_set<std::string>> HeaderCandidates;
  const std::regex ActorMacroPattern(R"(DEFINE_ACTOR_CLASS\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\))");
  std::error_code ErrorCode;

  for (const char* SearchRootName : {"Engine", "Game", "Launcher"}) {
    const std::filesystem::path SearchRoot = ProjectRoot / SearchRootName;
    if (!std::filesystem::is_directory(SearchRoot, ErrorCode)) {
      ErrorCode.clear();
      continue;
    }

    std::filesystem::recursive_directory_iterator Iterator(
        SearchRoot, std::filesystem::directory_options::skip_permission_denied, ErrorCode
    );
    const std::filesystem::recursive_directory_iterator End;
    for (; Iterator != End; Iterator.increment(ErrorCode)) {
      if (ErrorCode) {
        ErrorCode.clear();
        continue;
      }
      if (!Iterator->is_regular_file(ErrorCode) || !IsSearchableHeader(Iterator->path())) {
        ErrorCode.clear();
        continue;
      }

      std::string Content;
      if (!ReadTextFile(Iterator->path(), Content)) {
        continue;
      }
      for (std::sregex_iterator Match(Content.begin(), Content.end(), ActorMacroPattern);
           Match != std::sregex_iterator();
           ++Match) {
        const std::string ClassName = (*Match)[1].str();
        if (ClassName != "ClassName") {
          HeaderCandidates[ClassName].insert(Iterator->path().string());
        }
      }
    }
  }

  for (const auto& [ClassName, Paths] : HeaderCandidates) {
    if (Paths.size() == 1) {
      ActorHeaderIndex.emplace(
          ClassName, std::filesystem::path(*Paths.begin()).filename().string()
      );
    } else {
      AmbiguousActorClasses.insert(ClassName);
    }
  }

  if (ActorHeaderIndex.empty() && AmbiguousActorClasses.empty()) {
    ErrorMessage = "Actor class definitions were not found under the project root.";
    return false;
  }

  IndexedProjectRoot = ProjectRoot;
  bActorHeaderIndexBuilt = true;
  return true;
}

FCreateNewActorResult ActorClassGenerator::Generate(const FCreateNewActorRequest& Request) {
  if (Request.ClassName.empty()) {
    return Fail("Class Name is required.");
  }
  if (!IsCppIdentifier(Request.ClassName)) {
    return Fail("Class Name must be a valid C++ identifier.");
  }
  if (IsCppKeyword(Request.ClassName)) {
    return Fail("Class Name cannot be a C++ keyword.");
  }
  if (Request.ParentClassName.empty()) {
    return Fail("Parent Actor Class is required.");
  }

  const auto& RegisteredClasses = ActorRegistry::GetInstance().GetClassNames();
  if (std::find(RegisteredClasses.begin(), RegisteredClasses.end(), Request.ParentClassName) ==
      RegisteredClasses.end()) {
    return Fail("The selected parent is not registered as a normal Actor class.");
  }
  if (ActorRegistry::GetInstance().Contains(Request.ClassName)) {
    return Fail("An Actor class with the same name is already registered.");
  }
  if (Request.OutputDirectory.empty()) {
    return Fail("Output Path is required.");
  }

  const std::filesystem::path ProjectRoot = FindProjectRoot();
  if (ProjectRoot.empty()) {
    return Fail("Could not locate the project root or Create New Actor templates.");
  }

  std::string IndexError;
  if (!bActorHeaderIndexBuilt || IndexedProjectRoot != ProjectRoot) {
    if (!BuildActorHeaderIndex(ProjectRoot, IndexError)) {
      return Fail(std::move(IndexError));
    }
  }
  if (AmbiguousActorClasses.contains(Request.ParentClassName)) {
    return Fail("The parent Actor class is defined in more than one header.");
  }
  const auto HeaderIterator = ActorHeaderIndex.find(Request.ParentClassName);
  if (HeaderIterator == ActorHeaderIndex.end()) {
    return Fail("Could not resolve the header for the parent Actor class.");
  }

  std::error_code ErrorCode;
  std::filesystem::path OutputDirectory = Request.OutputDirectory;
  if (OutputDirectory.is_relative()) {
    const std::filesystem::path StartupDirectory = std::filesystem::current_path(ErrorCode);
    if (ErrorCode) {
      return Fail("Could not resolve the startup directory.");
    }
    OutputDirectory = StartupDirectory / OutputDirectory;
  }
  std::filesystem::create_directories(OutputDirectory, ErrorCode);
  if (ErrorCode || !std::filesystem::is_directory(OutputDirectory, ErrorCode)) {
    return Fail("Output Path does not exist and could not be created.");
  }

  const std::filesystem::path HeaderPath = OutputDirectory / (Request.ClassName + ".h");
  const std::filesystem::path SourcePath = OutputDirectory / (Request.ClassName + ".cpp");
  if (std::filesystem::exists(HeaderPath, ErrorCode)) {
    return Fail("A header file with the same name already exists.");
  }
  ErrorCode.clear();
  if (std::filesystem::exists(SourcePath, ErrorCode)) {
    return Fail("A source file with the same name already exists.");
  }

  const std::filesystem::path TemplateDirectory = ProjectRoot / TemplateRelativePath;
  const std::filesystem::path HeaderTemplate =
      FindTemplate(TemplateDirectory, Request.ParentClassName, ".h");
  const std::filesystem::path SourceTemplate =
      FindTemplate(TemplateDirectory, Request.ParentClassName, ".cpp");

  std::string HeaderContent;
  std::string SourceContent;
  if (!ReadTextFile(HeaderTemplate, HeaderContent)) {
    return Fail("Failed to read the header template: " + HeaderTemplate.string());
  }
  if (!ReadTextFile(SourceTemplate, SourceContent)) {
    return Fail("Failed to read the source template: " + SourceTemplate.string());
  }

  for (std::string* Content : {&HeaderContent, &SourceContent}) {
    ReplaceAll(*Content, "{CLASS_NAME}", Request.ClassName);
    ReplaceAll(*Content, "{PARENT_CLASS}", Request.ParentClassName);
    ReplaceAll(*Content, "{PARENT_HEADER}", HeaderIterator->second);
  }

  const std::filesystem::path HeaderTemporaryPath = HeaderPath.string() + ".tmp";
  const std::filesystem::path SourceTemporaryPath = SourcePath.string() + ".tmp";
  if (std::filesystem::exists(HeaderTemporaryPath, ErrorCode) ||
      std::filesystem::exists(SourceTemporaryPath, ErrorCode)) {
    return Fail("A temporary generation file already exists in Output Path.");
  }

  const auto CleanupTemporaryFiles = [&]() {
    std::error_code CleanupError;
    std::filesystem::remove(HeaderTemporaryPath, CleanupError);
    CleanupError.clear();
    std::filesystem::remove(SourceTemporaryPath, CleanupError);
  };

  if (!WriteTextFile(HeaderTemporaryPath, HeaderContent) ||
      !WriteTextFile(SourceTemporaryPath, SourceContent)) {
    CleanupTemporaryFiles();
    return Fail("Failed to write the temporary Actor class files.");
  }

  std::filesystem::rename(HeaderTemporaryPath, HeaderPath, ErrorCode);
  if (ErrorCode) {
    CleanupTemporaryFiles();
    return Fail("Failed to finalize the generated header file.");
  }
  std::filesystem::rename(SourceTemporaryPath, SourcePath, ErrorCode);
  if (ErrorCode) {
    std::error_code CleanupError;
    std::filesystem::remove(HeaderPath, CleanupError);
    CleanupTemporaryFiles();
    return Fail("Failed to finalize the generated source file.");
  }

  FCreateNewActorResult Result;
  Result.bSuccess = true;
  Result.HeaderPath = std::filesystem::absolute(HeaderPath);
  Result.SourcePath = std::filesystem::absolute(SourcePath);
  Result.Message =
      "C++ Actor class generated successfully.\n\n"
      "To use the generated code, rebuild the project with Visual Studio or MSBuild,\n"
      "then restart the engine.\n\nHeader: " +
      Result.HeaderPath.string() + "\nSource: " + Result.SourcePath.string();
  return Result;
}
