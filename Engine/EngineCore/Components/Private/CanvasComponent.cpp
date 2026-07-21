#include "CanvasComponent.h"

#include <DxLib.h>

#include <cmath>

MCanvasComponent::MCanvasComponent() {
  // デフォルトの描画設定（優先度0、ワールド空間）
  SetRenderSettings(0, RenderSpace::World);
}

MCanvasComponent::~MCanvasComponent() { ReleaseCanvas(); }

bool MCanvasComponent::CreateCanvas(int InWidth, int InHeight, bool bUseAlpha) {
  ReleaseCanvas();

  if (InWidth <= 0 || InHeight <= 0) {
    return false;
  }

  // 現在の描画先を退避してからMakeScreenを呼ぶ
  int PrevScreen = GetDrawScreen();
  SetDrawScreen(DX_SCREEN_BACK);

  // DxLib の MakeScreen により描画可能なレンダーターゲットテクスチャを生成
  CanvasHandle = MakeScreen(InWidth, InHeight, bUseAlpha ? TRUE : FALSE);

  // 描画先を元に戻す
  SetDrawScreen(PrevScreen);

  if (CanvasHandle == -1) {
    return false;
  }

  Width = InWidth;
  Height = InHeight;

  // MSpriteComponent のグラフィックハンドルとして登録
  SubmitGraph(CanvasHandle);

  return true;
}

void MCanvasComponent::ReleaseCanvas() {
  if (CanvasHandle != -1) {
    DeleteGraph(CanvasHandle);
    CanvasHandle = -1;
  }
  Width = 0;
  Height = 0;
}

bool MCanvasComponent::BeginDrawing() {
  if (CanvasHandle == -1) {
    return false;
  }

  // 現在の描画ターゲットを保存し、ターゲットをこのキャンバスに切り替える
  SavedDrawScreen = GetDrawScreen();
  if (SetDrawScreen(CanvasHandle) != 0) {
    SavedDrawScreen = -1;
    return false;
  }
  return true;
}

void MCanvasComponent::EndDrawing() {
  if (SavedDrawScreen != -1) {
    SetDrawScreen(SavedDrawScreen);
    SavedDrawScreen = -1;
  }
}

void MCanvasComponent::Clear(const FColor& Color) {
  if (CanvasHandle == -1) return;

  // ブレンドモードを変更して、アルファ値も含めて背景色で強制上書きクリア
  int PrevBlendMode = 0;
  int PrevAlpha = 0;
  GetDrawBlendMode(&PrevBlendMode, &PrevAlpha);

  SetDrawBlendMode(DX_BLENDMODE_SRCCOLOR, 255);
  DxLib::DrawBox(0, 0, Width, Height, Color.ToRGB(), TRUE);

  SetDrawBlendMode(PrevBlendMode, PrevAlpha);
}

void MCanvasComponent::DrawPixel(const FVector2D& LocalPosition, const FColor& Color) {
  DxLib::DrawPixel(
      static_cast<int>(LocalPosition.X), static_cast<int>(LocalPosition.Y), Color.ToRGB()
  );
}

void MCanvasComponent::DrawLine(
    const FVector2D& LocalStart, const FVector2D& LocalEnd, const FColor& Color, int Thickness
) {
  DxLib::DrawLine(
      static_cast<int>(LocalStart.X),
      static_cast<int>(LocalStart.Y),
      static_cast<int>(LocalEnd.X),
      static_cast<int>(LocalEnd.Y),
      Color.ToRGB(),
      Thickness
  );
}

void MCanvasComponent::DrawCircle(
    const FVector2D& LocalCenter, float Radius, const FColor& Color, bool bFill
) {
  DxLib::DrawCircle(
      static_cast<int>(LocalCenter.X),
      static_cast<int>(LocalCenter.Y),
      static_cast<int>(Radius),
      Color.ToRGB(),
      bFill ? TRUE : FALSE
  );
}

void MCanvasComponent::DrawBox(
    const FVector2D& LocalTopLeft, const FVector2D& Size, const FColor& Color, bool bFill
) {
  DxLib::DrawBox(
      static_cast<int>(LocalTopLeft.X),
      static_cast<int>(LocalTopLeft.Y),
      static_cast<int>(LocalTopLeft.X + Size.X),
      static_cast<int>(LocalTopLeft.Y + Size.Y),
      Color.ToRGB(),
      bFill ? TRUE : FALSE
  );
}

FVector2D MCanvasComponent::WorldToCanvasLocal(const FVector2D& WorldPosition) const {
  if (Width <= 0 || Height <= 0) {
    return FVector2D::ZeroVector();
  }

  // 親コンポーネントおよび自身のワールド変換から相対座標を求める
  const FVector2D CanvasWorldPos = GetWorldLocation();
  const FRotator CanvasWorldRot = GetWorldRotation();
  const FScale CanvasWorldScale = GetWorldScale();

  if (std::abs(CanvasWorldScale.Scale) < 1e-6f) {
    return FVector2D::ZeroVector();
  }

  // 回転の逆変換
  const float Rad = UMath::DegToRad(CanvasWorldRot.Rotation);
  const float CosR = std::cos(Rad);
  const float SinR = std::sin(Rad);

  const float DiffX = WorldPosition.X - CanvasWorldPos.X;
  const float DiffY = WorldPosition.Y - CanvasWorldPos.Y;

  // キャンバスのローカル座標（ワールドスケール考慮）
  const float LocalX = (DiffX * CosR + DiffY * SinR) / CanvasWorldScale.Scale;
  const float LocalY = (-DiffX * SinR + DiffY * CosR) / CanvasWorldScale.Scale;

  // キャンバスの左上原点（0 ~ Width, 0 ~ Height）に変換
  return {LocalX + Width * 0.5f, LocalY + Height * 0.5f};
}
