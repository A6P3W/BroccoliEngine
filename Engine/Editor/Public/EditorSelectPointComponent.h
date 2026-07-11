#pragma once
#include "SceneComponent.h"

class MSpriteComponent;
class EditorSelectPointComponent : public MSceneComponent {
 public:
  EditorSelectPointComponent();

  void Draw() override;
  void Selected(bool bSelected);

 private:
  void OnRegister() override;
  MSpriteComponent* SelectPointSprite = nullptr;
};
