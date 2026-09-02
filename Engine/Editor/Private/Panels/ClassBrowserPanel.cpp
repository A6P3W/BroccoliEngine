#include "Panels/ClassBrowserPanel.h"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "EditorContext.h"
#include "EditorMode.h"

void ClassBrowserPanel::DrawContents(EditorContext& Context) {
  EditorMode* Mode = Context.Mode;
  const std::string& SelectedClass = Mode->GetSelectedClass();
  std::vector<std::string> SortedClasses = Mode->GetClassList();
  std::sort(SortedClasses.begin(), SortedClasses.end());

  const auto OnClassSelected = [&](const std::string& ClassName) {
    Mode->SelectClass(ClassName);
    const auto ExistingClass = std::find(RecentClasses.begin(), RecentClasses.end(), ClassName);
    if (ExistingClass != RecentClasses.end()) {
      RecentClasses.erase(ExistingClass);
    }
    RecentClasses.insert(RecentClasses.begin(), ClassName);
    if (RecentClasses.size() > 5) RecentClasses.pop_back();
  };

  ImGui::Text("Selected:");
  ImGui::SameLine();
  if (SelectedClass.empty()) {
    ImGui::TextDisabled("(None)");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "[ %s ]", SelectedClass.c_str());
  }

  if (ImGui::Button("Clear Selection", ImVec2(-1.0f, 0.0f))) Mode->SelectClass("");
  ImGui::Separator();

  static ImGuiTextFilter Filter;
  Filter.Draw("##Search", ImGui::GetContentRegionAvail().x);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Search classes...");

  ImGui::SliderInt("Group Length", &GroupCharacterCount, 1, 4, "%d chars");
  ImGui::Separator();

  const auto GetHueFromName = [](const std::string& Name) {
    unsigned long Hash = 5381;
    for (const char Character : Name) Hash = ((Hash << 5) + Hash) + Character;
    return static_cast<float>(Hash % 1000) / 1000.0f;
  };

  const auto DrawClassSelectable = [&](const std::string& ClassName) {
    const bool IsSelected = ClassName == SelectedClass;
    float Red = 0.0f;
    float Green = 0.0f;
    float Blue = 0.0f;
    ImGui::ColorConvertHSVtoRGB(GetHueFromName(ClassName), 0.4f, 0.9f, Red, Green, Blue);

    if (!IsSelected) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(Red, Green, Blue, 1.0f));
    if (ImGui::Selectable(ClassName.c_str(), IsSelected)) OnClassSelected(ClassName);
    if (!IsSelected) ImGui::PopStyleColor();
    if (IsSelected) ImGui::SetItemDefaultFocus();
  };

  ImGui::BeginChild(
      "ClassListRegion", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar
  );
  if (Filter.IsActive()) {
    for (const std::string& ClassName : SortedClasses) {
      if (Filter.PassFilter(ClassName.c_str())) DrawClassSelectable(ClassName);
    }
  } else {
    if (!RecentClasses.empty()) {
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);
      if (ImGui::TreeNode("Recent")) {
        for (const std::string& ClassName : RecentClasses) {
          if (std::find(SortedClasses.begin(), SortedClasses.end(), ClassName) !=
              SortedClasses.end()) {
            DrawClassSelectable(ClassName);
          }
        }
        ImGui::TreePop();
      }
      ImGui::Separator();
    }

    const auto GetGroupKey = [&](const std::string& Name) {
      if (Name.empty()) return std::string("?");

      const size_t CharacterCount =
          (std::min)(Name.length(), static_cast<size_t>(GroupCharacterCount));
      std::string Key = Name.substr(0, CharacterCount);
      for (char& Character : Key) {
        Character = static_cast<char>(std::toupper(static_cast<unsigned char>(Character)));
      }
      return Key;
    };

    std::string CurrentGroup;
    bool NodeOpen = false;
    for (const std::string& ClassName : SortedClasses) {
      const std::string GroupKey = GetGroupKey(ClassName);
      if (GroupKey != CurrentGroup) {
        if (!CurrentGroup.empty() && NodeOpen) ImGui::TreePop();
        CurrentGroup = GroupKey;
        const std::string HeaderName = "[ " + CurrentGroup + " ]";
        NodeOpen = ImGui::TreeNodeEx(HeaderName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
      }
      if (NodeOpen) DrawClassSelectable(ClassName);
    }
    if (!CurrentGroup.empty() && NodeOpen) ImGui::TreePop();
  }

  ImGui::EndChild();
}
