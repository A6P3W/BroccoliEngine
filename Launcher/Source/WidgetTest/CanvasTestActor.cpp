#include "CanvasTestActor.h"

#include <DxLib.h>

#include "CameraComponent.h"
#include "CanvasComponent.h"
#include "EnhancedInputComponent.h"
#include "InputManager.h"
#include "InputMapper.h"
#include "Log.h"
#include "MouseDevice.h"
#include "RenderSystem.h"

// このアクタークラスを ActorRegistry に一般アクターとして自動登録するマクロ
REGISTER_ACTOR(ACanvasTestActor)

ACanvasTestActor::ACanvasTestActor() {
  // キャンバスコンポーネントを作成してルートにアタッチ
  Canvas = NewObject<MCanvasComponent>(this);
  if (Canvas) {
    Canvas->AttachToComponent(GetRootComponent());
    Canvas->RegisterComponent();
  }
}

void ACanvasTestActor::BeginPlay() {
  APawn::BeginPlay();
  if (Canvas) {
    // 512x512 の半透明対応キャンバスを生成
    if (Canvas->CreateCanvas(512, 512, true)) {
      // 白でクリア
      if (Canvas->BeginDrawing()) {
        Canvas->Clear(FColor{255, 255, 255, 255});
        Canvas->EndDrawing();
      }
      M_LOG("CanvasTestActor: Canvas created successfully (512x512).");
    } else {
      M_LOG("CanvasTestActor: Failed to create canvas.");
    }
  }
}

void ACanvasTestActor::OnPossessedBy(APlayerController* NewController) {
  APawn::OnPossessedBy(NewController);
  Camera->SetFOV(1);
}

void ACanvasTestActor::OnUpdate(float DeltaTime) {
  APawn::OnUpdate(DeltaTime);

  auto* Mouse = InputManager::GetInstance().GetDevice<MouseDevice>();
  if (!Mouse || !Canvas) return;

  // 現在のマウスの仮想スクリーン座標を取得
  FVector2D ScreenPos{
      static_cast<float>(Mouse->GetMouseX()),
      static_cast<float>(Mouse->GetMouseY()),
  };

  // 仮想スクリーン座標 → ワールド座標への変換
  FVector2D WorldMousePos = RenderSystem::GetInstance().ScreenToWorld(ScreenPos);

  // ワールド座標 → キャンバス内のローカルピクセル座標へ変換
  FVector2D LocalPaintPos = Canvas->WorldToCanvasLocal(WorldMousePos);

  // デバッグ情報をログに出力（毎フレームは冗長なのでコメントアウト可能）
  // M_LOG("Canvas Mouse: Screen({:.0f},{:.0f}) Local({:.0f},{:.0f})",
  //     ScreenPos.X, ScreenPos.Y, LocalPaintPos.X, LocalPaintPos.Y);

  // 範囲内かつ左クリック中の場合、キャンバスに描画
  if (Mouse->GetPressing(MOUSE_INPUT_LEFT)) {
    if (LocalPaintPos.X >= 0 && LocalPaintPos.X < Canvas->GetCanvasWidth() &&
        LocalPaintPos.Y >= 0 && LocalPaintPos.Y < Canvas->GetCanvasHeight()) {
      if (Canvas->BeginDrawing()) {
        // 前回の座標からの線で繋ぐと滑らかな描画になる
        if (bIsPaintingLastFrame) {
          Canvas->DrawLine(LastPaintPos, LocalPaintPos, FColor{255, 0, 0}, 5);
        } else {
          Canvas->DrawCircle(LocalPaintPos, 2.5f, FColor{255, 0, 0}, true);
        }
        Canvas->EndDrawing();
      }

      LastPaintPos = LocalPaintPos;
      bIsPaintingLastFrame = true;
    } else {
      bIsPaintingLastFrame = false;
    }
  } else {
    bIsPaintingLastFrame = false;
  }

  // 右クリックでキャンバスをクリア
  if (Mouse->GetPressStart(MOUSE_INPUT_RIGHT)) {
    if (Canvas->BeginDrawing()) {
      Canvas->Clear(FColor{255, 255, 255, 255});
      Canvas->EndDrawing();
    }
    M_LOG("CanvasTestActor: Canvas cleared.");
  }
}

void ACanvasTestActor::SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) {
  PlayerInputComponent->BindAction(
      InputAction::Move, ETriggerEvent::Triggered, this, &ACanvasTestActor::OnMove
  );
}

void ACanvasTestActor::OnMove(const FInputActionValue& Value) {
  // カメラ移動は不要。キャンバスのテストに集中。
}
