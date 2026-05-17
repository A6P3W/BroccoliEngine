#pragma once
#include "ActorComponent.h"
#include "InputMapper.h"
#include <functional>
#include <vector>
#include "Utils/Umath.h"
struct FInputActionValue {
	FVector2D Axis2D = FVector2D::ZeroVector; // スティック、WASDの複合ベクトル
	bool bIsPressed = false;                   // ボタンのON/OFF
};
class MEnhancedInputComponent :public MActorComponent
{
private:
	struct FInputBinding {
		E_INPUT_ACTION Action;
		ETriggerEvent Event;
		std::function<void(const FInputActionValue&)> Callback;
	};
	std::vector<FInputBinding> Bindings;

public:
	// EnhancedInputComponent.h の拡張

	// パターン①：引数ありの関数（OnMoveなど）を受け取るBindAction
	template<class UserClass>
	void BindAction(E_INPUT_ACTION Action, ETriggerEvent Event, UserClass* Object, void (UserClass::* Func)(const FInputActionValue&)) {
		Bindings.push_back({
			Action,
			Event,
			// ラムダ式が引数を受け取り、そのまま対象メンバ関数に渡す
			[Object, Func](const FInputActionValue& Value) { (Object->*Func)(Value); }
			});
	}

	// パターン②：引数なしの関数（OnReloadPressedなど）を受け取るBindAction（★追加）
	template<class UserClass>
	void BindAction(E_INPUT_ACTION Action, ETriggerEvent Event, UserClass* Object, void (UserClass::* Func)()) {
		Bindings.push_back({
			Action,
			Event,
			// ラムダ式は引数を受け取るが、対象メンバ関数を呼ぶときは引数を無視する
			[Object, Func](const FInputActionValue& Value) { (Object->*Func)(); }
			});
	}

	// 毎フレーム Application または Actor から呼び出してイベントを発火させる
	void ProcessInputBindings();
};
