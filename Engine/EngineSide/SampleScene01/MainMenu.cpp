#include "MainMenu.h"
#include "UIButtonComponent.h"
#include "Log.h"
MainMenu::MainMenu()
{
    auto StartBtn = std::make_unique<MUIButtonComponent>();
	auto StartBtnPtr = StartBtn.get();
    AddComponent(std::move(StartBtn));

    auto ConfigBtn = std::make_unique<MUIButtonComponent>();
	auto ConfigBtnPtr = ConfigBtn.get();
    AddComponent(std::move(ConfigBtn));
    auto ExitBtn = std::make_unique<MUIButtonComponent>();
	auto ExitBtnPtr = ExitBtn.get();
    AddComponent(std::move(ExitBtn));

    // --- ナビゲーションの手動リンク設定 ---

    StartBtnPtr->Navigation.Down = ConfigBtnPtr;

    ConfigBtnPtr->Navigation.Up = StartBtnPtr;
    ConfigBtnPtr->Navigation.Down = ExitBtnPtr;

    ExitBtnPtr->Navigation.Up = ConfigBtnPtr;
    StartBtnPtr->OnPressed = []() {
        M_LOG("Start Game!");
        // SceneManager::GetInstance().OpenScene<GameScene>(); // シーン遷移などの処理
        };
    // 最初にフォーカスを当てるボタンを設定
    SetFocusedButton(StartBtnPtr);
}
