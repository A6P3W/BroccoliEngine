#pragma once
#include "SceneComponent.h"

class MSprite2DComponent;
class EditorSelectPointComponent : public MSceneComponent {
 public:
  EditorSelectPointComponent();

  void Draw() override;
  void Selected(bool bSelected);

 private:
  void OnRegister() override;
  MSprite2DComponent* SelectPointSprite = nullptr;
};
