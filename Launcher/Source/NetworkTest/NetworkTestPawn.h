#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "ActorComponent.h"
#include "Color.h"
#include "NetworkTestRepComponent.h"
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
class MSpriteComponent;
struct FInputActionValue;
class MNetMovementComponent;
class FNetworkTestUI;
class MNetworkTestRepComponent;

// プレイヤーやAIが憑依して操作可能な、エンジンのポーン基底クラス APawn を継承したネットワーク同期テスト用の Pawn クラス
class ANetworkTestPawn : public APawn {
 public:
  // クラスのメタデータ（型名 StaticClassName(), クラス名取得 GetActorClassName() など）を定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ANetworkTestPawn)
  ANetworkTestPawn();
  ~ANetworkTestPawn() override;

  // アクターがゲーム内に配置・生成され、プレイが開始されたときに一度だけ呼び出されるライフサイクル関数
  void BeginPlay() override;

  // プレイヤーコントローラーがこの Pawn に憑依（Possess）した際に呼び出されるライフサイクル関数
  void OnPossessedBy(APlayerController* NewController) override;

  // 毎フレーム呼び出される更新用ライフサイクル関数。DeltaTime は前フレームからの経過時間（秒）
  void OnUpdate(float DeltaTime) override;

  // 描画処理用のライフサイクル関数（UIの描画などを処理）
  void Draw() override;

  // プレイヤーからの入力をこの Pawn のアクションにバインドする設定用ライフサイクル関数
  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

  void SetStatusMessage(const std::string& Message);

  // リスンサーバーとしてマルチプレイセッションを起動し、他クライアントの待受を開始する設定関数
  bool StartListenServer(uint16_t Port);

  // クライアントとして稼働しているサーバーへネットワーク接続を行う設定関数
  bool ConnectAsClient(const std::string& HostAddress, uint16_t Port);

 private:
  void OnMove(const FInputActionValue& Value);
  void OnInteract(const FInputActionValue& Value);
  // 他のアクターのコリジョンと重なりが生じた際にエンジンから呼び出されるコールバック関数
  void BeginOverlap(AActor* OtherActor) override;
  FColor GetDisplayColor() const;

  MSpriteComponent* BodySprite = nullptr;
  MNetMovementComponent* Movement = nullptr;
  MNetworkTestRepComponent* ReplicationTest = nullptr;
  std::unique_ptr<FNetworkTestUI> NetworkTestUI;
  float FlashTimer = 0.0f;

  bool bSessionStarted = false;
};
