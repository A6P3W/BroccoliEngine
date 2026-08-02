#pragma once

#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "SpriteComponent.h"
#include "UMath.h"

/**
 * 実行時に動的なテクスチャの生成と直接描画を行うキャンバスコンポーネント。
 * raylibのRenderTextureは内部ハンドルで管理し、公開APIには露出させない。
 */
class BROCCOLI_ENGINE_API MCanvasComponent : public MSpriteComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MCanvasComponent)
  MCanvasComponent();
  ~MCanvasComponent() override;

  bool CreateCanvas(int InWidth, int InHeight, bool UseAlpha = true);
  void ReleaseCanvas();

  bool BeginDrawing();
  void EndDrawing();

  void Clear(const FColor& Color);
  void DrawPixel(const FVector2D& LocalPosition, const FColor& Color);
  void DrawLine(
      const FVector2D& LocalStart, const FVector2D& LocalEnd, const FColor& Color, int Thickness = 2
  );
  void DrawCircle(
      const FVector2D& LocalCenter, float Radius, const FColor& Color, bool Fill = true
  );
  void DrawBox(
      const FVector2D& LocalTopLeft, const FVector2D& Size, const FColor& Color, bool Fill = true
  );

  FVector2D WorldToCanvasLocal(const FVector2D& WorldPosition) const;

  int GetCanvasWidth() const { return Width; }
  int GetCanvasHeight() const { return Height; }

 private:
  int CanvasHandle = 0;
  int Width = 0;
  int Height = 0;
  bool Drawing = false;
};
