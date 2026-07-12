#pragma once
#include "BroccoliEngineAPI.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "InputDevice.h"
#include "UMath.h"
struct InputAction {
  static constexpr auto Move = "Move";
  static constexpr auto Look = "Look";

  static constexpr auto Interact = "Interact";
  static constexpr auto Cancel = "Cancel";
  static constexpr auto Pause = "Pause";
};
struct InputActionLower {
  static constexpr auto MoveX = "MoveX";
  static constexpr auto MoveY = "MoveY";
  static constexpr auto LookX = "LookX";
  static constexpr auto LookY = "LookY";
};
struct InputActionMouse {
  static constexpr auto MouseLeft = "MouseLeft";
  static constexpr auto MouseRight = "MouseRight";
  static constexpr auto Wheel = "Wheel";
};

struct EditorInputAction {
  static constexpr auto SelectMode = "SelectMode";
  static constexpr auto MoveMode = "MoveMode";
  static constexpr auto RotateMode = "RotateMode";
  static constexpr auto ScaleMode = "ScaleMode";
  static constexpr auto ModifierCtrl = "ModifierCtrl";
  static constexpr auto Copy = "EditorCopy";
  static constexpr auto Paste = "EditorPaste";
  static constexpr auto Cut = "EditorCut";
};
struct UIAction {
  static constexpr auto Move = "UI_Move";
  static constexpr auto Submit = "UI_Submit";
  static constexpr auto Cancel = "UI_Cancel";
};
struct UIActionLower {
  static constexpr auto MoveX = "UI_MoveX";
  static constexpr auto MoveY = "UI_MoveY";
};

class BROCCOLI_ENGINE_API InputMapper {
 public:
  InputMapper() = default;

  void AddMapping(
      const std::string& actionName,
      InputDevice* device,
      int code,
      const std::string& modifierAction = "",
      float scale = 1.0f
  );

  void AddAxisMapping(
      const std::string& actionName, InputDevice* device, int axisId, float scale = 1.0f
  );

  void RemoveMapping(const std::string& actionName);

  bool GetPressStart(const std::string& actionName) const;
  bool GetPressing(const std::string& actionName) const;
  bool GetRelease(const std::string& actionName) const;
  float GetAxisValue(const std::string& actionName) const;
  FVector2D GetAxis2DValue(const std::string& actionNameX, const std::string& actionNameY) const;

 private:
  struct FButtonBinding {
    InputDevice* Device;
    int Code;
    float Scale;
    std::string ModifierAction;
  };
  struct FAxisBinding {
    InputDevice* Device;
    int AxisId;
    float Scale;
  };
  std::unordered_map<std::string, std::vector<FButtonBinding>> ButtonBindings;
  std::unordered_map<std::string, std::vector<FAxisBinding>> AxisBindings;
};
