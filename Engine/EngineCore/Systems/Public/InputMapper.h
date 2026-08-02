#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "InputDevice.h"
#include "InputTypes.h"
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

class GamepadDevice;
class KeyboardDevice;
class MouseDevice;

class BROCCOLI_ENGINE_API InputMapper {
 public:
  InputMapper();
  ~InputMapper();
  InputMapper(const InputMapper&) = delete;
  InputMapper& operator=(const InputMapper&) = delete;

  void AddMapping(
      const std::string& ActionName,
      KeyboardDevice* Device,
      EKey Key,
      const std::string& ModifierAction = "",
      float Scale = 1.0f
  );
  void AddMapping(
      const std::string& ActionName,
      MouseDevice* Device,
      EMouseButton Button,
      const std::string& ModifierAction = "",
      float Scale = 1.0f
  );
  void AddMapping(
      const std::string& ActionName,
      GamepadDevice* Device,
      EGamepadButton Button,
      const std::string& ModifierAction = "",
      float Scale = 1.0f
  );

  void AddAxisMapping(
      const std::string& ActionName, InputDevice* Device, int AxisId, float Scale = 1.0f
  );
  void RemoveMapping(const std::string& ActionName);

  bool GetPressStart(const std::string& ActionName) const;
  bool GetPressing(const std::string& ActionName) const;
  bool GetRelease(const std::string& ActionName) const;
  float GetAxisValue(const std::string& ActionName) const;
  FVector2D GetAxis2DValue(const std::string& ActionNameX, const std::string& ActionNameY) const;
  EInputDeviceType GetPressStartDevice(const std::string& ActionName) const;
  EInputDeviceType GetPressingDevice(const std::string& ActionName) const;
  EInputDeviceType GetReleaseDevice(const std::string& ActionName) const;
  EInputDeviceType GetAxisValueDevice(const std::string& ActionName) const;
  EInputDeviceType GetAxis2DValueDevice(
      const std::string& ActionNameX, const std::string& ActionNameY
  ) const;

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

  void AddButtonMapping(
      const std::string& ActionName,
      InputDevice* Device,
      int Code,
      const std::string& ModifierAction,
      float Scale
  );

  struct Impl;
  Impl* ImplPtr = nullptr;
};
