#pragma once
#include "BroccoliEngineAPI.h"
#include <functional>
#include <string>

#include "ActorComponent.h"
#include "InputMapper.h"
#include "UMath.h"

enum class ETriggerEvent { Started, Triggered, Completed };

struct FInputActionValue {
  FVector2D Axis2D = FVector2D::ZeroVector();
  float Axis1D = 0.0f;
  bool bIsPressed = false;
};

class BROCCOLI_ENGINE_API MEnhancedInputComponent : public MActorComponent {
 public:
  MEnhancedInputComponent();
  ~MEnhancedInputComponent() override;

  void ClearBindings();

  template <class T>
  void BindAction(
      const std::string& actionName,
      ETriggerEvent event,
      T* obj,
      void (T::*func)(const FInputActionValue&),
      bool isUIAction = false
  ) {
    AddBinding(
        actionName,
        event,
        [obj, func](const FInputActionValue& v) { (obj->*func)(v); },
        isUIAction
    );
  }
  template <class T>
  void BindAction(
      const std::string& actionName,
      ETriggerEvent event,
      T* obj,
      void (T::*func)(),
      bool isUIAction = false
  ) {
    AddBinding(
        actionName, event, [obj, func](const FInputActionValue&) { (obj->*func)(); }, isUIAction
    );
  }

  void ProcessInputBindings(const InputMapper& mapper, bool AllowUI, bool AllowGame);

 private:
  void AddBinding(
      const std::string& ActionName,
      ETriggerEvent Event,
      std::function<void(const FInputActionValue&)> Callback,
      bool bIsUIAction
  );

  struct Impl;
  Impl* ImplPtr = nullptr;
};