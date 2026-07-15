#pragma once
#include "BroccoliEngineAPI.h"
#include "RenderSystem.h"
#include "SceneComponent.h"

class SpriteRenderState;

class BROCCOLI_ENGINE_API MSpriteComponent : public MSceneComponent {
 public:
  MSpriteComponent();
  ~MSpriteComponent() override;
  void SetRenderSettings(int Priority, RenderSpace Space);

  void SubmitGraph(int handle, FScale scale = FScale(1.0f), int alpha = 255);
  void SubmitBox(float width, float height, const FColor& color, bool fill);
  void SubmitText(const std::string& text, const FColor& color, int handle = -1);
  void SubmitLine(const FVector2D& relativeEnd, const FColor& color);
  void SubmitRectGraph(float srcX, float srcY, float srcW, float srcH, int handle, int alpha = 255);
  void SubmitCircle(float radius, const FColor& color, bool fill);

  void Draw() override;

 private:
  SpriteRenderState* RenderState = nullptr;
};
