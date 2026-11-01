#pragma once

#include <cstdint>
#include <deque>
#include <nlohmann/json.hpp>
#include <string>

#include "BroccoliEngineAPI.h"

/// @brief メインスレッドにおけるエンジン更新の停止・再開・ステップ実行および非同期操作の完了状態を管理するクラス。
class BROCCOLI_ENGINE_API FEngineUpdateControl {
 public:
  /// @brief 初期の自動更新モードとステップ実行時の固定DeltaTimeを設定する。
  /// @param Running 起動時の自動更新
  /// @param FixedDeltaTime ステップ実行時および停止からの再開直後に使用する固定経過時間（秒）
  void Configure(bool Running, float FixedDeltaTime);

  /// @brief 1フレームの更新開始を試みる。
  /// @return 更新を実行してよい場合はtrue
  bool BeginUpdate();

  /// @brief 1フレームのEngineUpdate正常完了を記録し、通算更新回数やステップ消化数の加算・停止要求の確定を行う。
  void CompleteUpdate();

  /// @brief レベルロード直後停止などの理由で、更新回数を増やさずに現在のフレーム更新を中断して状態を戻す。
  void SkipUpdate();

  /// @brief シャットダウン状態へ移行し、未完了の操作をすべてcancelledにする。
  void Shutdown();

  /// @brief 終了要求フラグを立て、以降の新しい操作受付やフレーム更新の開始を止める。
  void RequestShutdown() { ShuttingDown = true; }

  /// @brief エンジンが終了処理中かどうかを返す。
  bool IsShuttingDown() const { return ShuttingDown; }

  /// @brief 現在ステップ実行の操作が進行中かどうかを返す。
  bool IsStepping() const { return !StepOperation.empty(); }

  /// @brief Actor生成・破棄やプロパティ変更などの変更操作が現在許可されているかを返す。
  bool CanMutate() const { return !ShuttingDown && !IsStepping(); }

  /// @brief 自動更新が有効かどうかを返す。
  bool IsRunning() const { return AutomaticUpdates; }

  /// @brief 現在1フレームの更新区間（BeginUpdate〜CompleteUpdate/SkipUpdate）を実行中かどうかを返す。
  bool IsUpdating() const { return Updating; }

  /// @brief 現在実行中のフレームがステップ実行として開始されたフレームかどうかを返す。
  bool IsStepUpdate() const { return !FrameStepOperation.empty(); }

  /// @brief ステップ実行時および停止からの再開直後に使用する固定DeltaTime（秒）を返す。
  float GetStepDeltaTime() const { return StepDeltaTime; }

  /// @brief 停止状態から再開した直後の経過時間リセット要求を取り出し、フラグを消費する。
  /// @return 再開直後の最初のフレームであり、固定DeltaTimeを使うべき場合はtrue
  bool TakeTimeReset();

  /// @brief 更新境界でのエンジン更新停止を要求し、受け付けた操作情報を返す。
  nlohmann::json Stop();

  /// @brief 自動更新を再開し、結果JSONを返す。
  nlohmann::json Start();

  /// @brief 停止中に指定フレーム数のステップ実行を要求し、受け付けた操作情報を返す。
  nlohmann::json Step(uint32_t Count);

  /// @brief 指定されたoperationIdの操作状態を取得する。
  nlohmann::json GetOperation(const std::string& OperationId) const;

  /// @brief 現在の更新制御状態スナップショット（stateレスポンスに追加されるJSONフィールド群）を取得する。
  nlohmann::json GetState() const;

  /// @brief Controlからのレベル切替要求を受け付け可能か。
  bool CanOpenLevel() const;

  /// @brief SceneManagerにキューされたレベル切替要求を追跡対象の非同期操作として登録する。
  /// @param RequestId SceneManagerが発行した保留中のリクエストID
  /// @param AfterUpdate ロード完了後の更新モード指定（"preserve", "run", "stop"）
  nlohmann::json TrackLevel(uint64_t RequestId, const std::string& AfterUpdate);

  /// @brief SceneManagerでのレベル切替処理の完了通知を受け取り、追跡中の操作状態を更新する。
  /// @param RequestId 処理されたSceneManagerのリクエストID
  /// @param Success レベルのロードに成功した場合はtrue
  /// @return ロード成功かつafterUpdate="stop"が指定されており、直後に更新を停止すべき場合はtrue
  bool OnSceneProcessed(uint64_t RequestId, bool Success);

  /// @brief 追跡中のレベル切替要求がロード前に別のレベル切替要求で上書きされていた場合、元の操作をfailedにする。
  /// @param PendingRequestId 現在SceneManagerに保留されている最新のリクエストID
  void FailSupersededLevel(uint64_t PendingRequestId);

 private:
  /// @brief 新しい非同期操作レコードを生成して履歴キューに追加する。
  nlohmann::json& NewOperation(const std::string& Kind, uint32_t Count = 0);

  /// @brief 履歴キューから指定したoperationIdの操作レコードを検索してポインタを返す。
  nlohmann::json* FindOperation(const std::string& OperationId);

  /// @brief 指定したoperationIdの操作を指定ステータス（completed / cancelled / failed）で終了させ、完了時点の更新番号を記録する。
  void Finish(
      const std::string& OperationId, const std::string& Status, const std::string& Reason = ""
  );

  /// @brief 更新境界において保留中の停止要求を完了させ、進行中のステップ実行があればcancelledにする。
  void SettleStop();

  bool ShuttingDown = false;              ///< 終了処理中フラグ
  bool AutomaticUpdates = true;           ///< 自動更新が有効か（running: true / stopped: false）
  bool Updating = false;                  ///< 1フレームの更新区間を実行中か
  bool ResetTime = false;                 ///< 再開直後の1フレームで固定DeltaTimeを使うフラグ
  bool StopAfterLevel = false;            ///< レベル切替完了直後に自動更新を停止するか
  float StepDeltaTime = 1.0f / 60.0f;     ///< ステップ実行・再開直後の固定DeltaTime（秒）
  uint64_t CompletedUpdates = 0;          ///< 正常完了したEngineUpdateの通算回数
  uint64_t SceneGeneration = 0;           ///< 成功したシーン（レベル）切替の通算回数
  uint64_t NextOperationId = 1;           ///< 次に発行するoperationIdの連番
  uint64_t LevelRequestId = 0;            ///< 現在追跡中のSceneManagerのレベル切替リクエストID
  std::string StepOperation;              ///< 進行中のステップ実行のoperationId（なければ空）
  std::string FrameStepOperation;         ///< 現フレーム開始時に確定したステップ実行のoperationId
  std::string StopOperation;              ///< 更新境界での完了待ちである停止操作のoperationId
  std::string LevelOperation;             ///< ロード完了待ちであるレベル切替操作のoperationId
  std::deque<nlohmann::json> Operations;  ///< 非同期操作の履歴キュー（直近256件＋未完了操作）
};
