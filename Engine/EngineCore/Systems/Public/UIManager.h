#pragma once
#include <vector>

#include "BroccoliEngineAPI.h"
#include "UMath.h"

struct FInputActionValue;
class AWidgetBase;

/// @brief UI管理クラス
class BROCCOLI_ENGINE_API UIManager {
 public:
  static UIManager* GetInstance();

  /// @brief WidgetにWidgetを追加する。
  /// @param Widget
  void AddWidget(AWidgetBase* Widget);
  /// @brief Widgetを削除する。
  /// @param Widget
  void RemoveWidget(AWidgetBase* Widget);

  /// @brief コントローラーからUIナビゲーション入力を受け取り、フォーカス中のWidgetに渡す。
  /// @param Value
  void Navigate(const FInputActionValue& Value);
  /// @brief コントローラーからUI決定入力を受け取り、フォーカス中のWidgetに渡す。
  /// @param Value
  void Submit(const FInputActionValue& Value);
  /// @brief コントローラーからUIキャンセル入力を受け取り、フォーカス中のWidgetに渡す。
  void Cancel();

  /// @brief UIでのテキスト入力を有効にする。ナビゲーションなどを停止。
  /// @param bActive
  void SetTextInputActive(bool bActive);
  /// @brief テキスト入力モードが有効か。
  /// @return
  bool IsTextInputActive() const;

  void SetFocusedWidget(AWidgetBase* Widget);
  /// @brief フォーカス中のWidgetを取得する。
  /// @return
  AWidgetBase* GetFocusedWidget() const;

 private:
  UIManager();
  ~UIManager();
  void RefreshZOrder();

  struct Impl;
  Impl* ImplPtr = nullptr;
};
