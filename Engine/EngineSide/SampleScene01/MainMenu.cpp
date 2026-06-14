#include "MainMenu.h"
#include "UIBoxButton.h"
#include "Log.h"
#include <DxLib.h>
#include "UITextComponent.h"
#include "Application.h"
MainMenu::MainMenu()
{
    float btnWidth = 200.0f;
    float btnHeight = 50.0f;
    int normalColor = GetColor(100, 100, 100);
    int hoveredColor = GetColor(150, 150, 150);
    int pressedColor = GetColor(50, 50, 50);

    // Startボタン
    auto StartBtn = std::make_unique<UIBoxButtonComponent>(btnWidth, btnHeight, normalColor, hoveredColor, pressedColor);
    auto StartBtnPtr = StartBtn.get();
    StartBtnPtr->SetAnchoredPosition({ 0.0f, -200.0f }); 
    AddComponent(std::move(StartBtn));

    // Configボタン
    auto ConfigBtn = std::make_unique<UIBoxButtonComponent>(btnWidth, btnHeight, normalColor, hoveredColor, pressedColor);
    auto ConfigBtnPtr = ConfigBtn.get();
    ConfigBtnPtr->SetAnchoredPosition({ 0.0f, 0.0f });
    AddComponent(std::move(ConfigBtn));

    // Exitボタン
    auto ExitBtn = std::make_unique<UIBoxButtonComponent>(btnWidth, btnHeight, normalColor, hoveredColor, pressedColor);
    auto ExitBtnPtr = ExitBtn.get();
    ExitBtnPtr->SetAnchoredPosition({ 0.0f, 200.0f });
    AddComponent(std::move(ExitBtn));

    // --- ナビゲーションの手動リンク設定 ---
    StartBtnPtr->Navigation.Down = ConfigBtnPtr;
    auto startText = std::make_unique<UITextComponent>("Start", GetColor(255, 255, 255), 24);
    startText->SetParentComponent(StartBtnPtr); // ボタンの子要素にするだけで中央に配置される
    AddComponent(std::move(startText));

    ConfigBtnPtr->Navigation.Up = StartBtnPtr;
    ConfigBtnPtr->Navigation.Down = ExitBtnPtr;

    ExitBtnPtr->Navigation.Up = ConfigBtnPtr;

    // ボタン押下時の処理
    StartBtnPtr->OnPressed = []() {
        M_LOG("Start Game!");
        };
	ExitBtnPtr->OnPressed = []() {
		M_LOG("Exit Game!");
		Application::QuitGame();

		};
    // 最初にフォーカスを当てるボタンを設定
    SetFocusedButton(StartBtnPtr);
}
