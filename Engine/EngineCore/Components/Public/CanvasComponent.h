#pragma once
#include "BroccoliEngineAPI.h"
#include "Color.h"
#include "SpriteComponent.h"
#include "UMath.h"

/**
 * 実行時に動的なテクスチャの生成と直接描画を行うキャンバスコンポーネント
 * MakeScreen で生成したレンダーターゲットを MSpriteComponent のスプライトとして描画する
 */
class BROCCOLI_ENGINE_API MCanvasComponent : public MSpriteComponent {
 public:
  MCanvasComponent();
  ~MCanvasComponent() override;

  // 動的テクスチャ（MakeScreen）の生成と解放
  bool CreateCanvas(int InWidth, int InHeight, bool bUseAlpha = true);
  void ReleaseCanvas();

  // キャンバスへの描画セッションの開始・終了
  bool BeginDrawing();
  void EndDrawing();

  // 描画APIのラップ
  void Clear(const FColor& Color);
  void DrawPixel(const FVector2D& LocalPosition, const FColor& Color);
  void DrawLine(
      const FVector2D& LocalStart, const FVector2D& LocalEnd, const FColor& Color, int Thickness = 1
  );
  void DrawCircle(
      const FVector2D& LocalCenter, float Radius, const FColor& Color, bool bFill = true
  );
  void DrawBox(
      const FVector2D& LocalTopLeft, const FVector2D& Size, const FColor& Color, bool bFill = true
  );

  // ワールド座標からキャンバス内のピクセル座標（左上原点 0~Width, 0~Height）への変換
  FVector2D WorldToCanvasLocal(const FVector2D& WorldPosition) const;

  int GetCanvasWidth() const { return Width; }
  int GetCanvasHeight() const { return Height; }

 private:
  int CanvasHandle = -1;
  int SavedDrawScreen = -1;
  int Width = 0;
  int Height = 0;
};
