#pragma once
#include "BroccoliEngineAPI.h"
#include <functional>

#include "UIButtonComponent.h"

class MSpriteComponent;

class BROCCOLI_ENGINE_API UIToggleButtonComponent : public MUIButtonComponent {
  public:
   UIToggleButtonComponent();
   ~UIToggleButtonComponent() override;
 
   void OnRegister() override;
   void Press() override;
   void OnStateChanged(EButtonState NewState) override;
 
   void SetIsOn(bool bNewIsOn, bool bBroadcast = true);
   bool GetIsOn() const;
 
   void SetOnToggled(std::function<void(bool)> Callback);
 
   void SetSize(float width, float height);
   void SetColors(
       int onNormal, int onHovered, int onPressed, int offNormal, int offHovered, int offPressed
   );
 
  private:
   void Toggle();
   void UpdateVisuals();
   int GetCurrentColor() const;
 
   struct Impl;
   Impl* ImplPtr = nullptr;
 };
