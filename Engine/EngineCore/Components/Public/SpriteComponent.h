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
  void SubmitBox(float width, float height, int color, bool fill, int alpha = 255);
  void SubmitText(const std::string& text, int color, int handle = -1, int alpha = 255);
  void SubmitLine(const FVector2D& relativeEnd, int color, int alpha = 255);
  void SubmitRectGraph(float srcX, float srcY, float srcW, float srcH, int handle, int alpha = 255);
  void SubmitCircle(float radius, int color, bool fill, int alpha = 255);

  void Draw() override;

 private:
  SpriteRenderState* RenderState = nullptr;
};
