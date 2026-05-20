#include "EnhancedInputComponent.h"

void MEnhancedInputComponent::ProcessInputBindings()
{
	auto& Mapper = InputMapper::GetInstance();

	for (const auto& Binding : Bindings) {
		bool bShouldTrigger = false;

		switch (Binding.Event) {
		case ETriggerEvent::Started:
			bShouldTrigger = Mapper.GetKeyPressStart(Binding.Action);
			break;
		case ETriggerEvent::Triggered:
			bShouldTrigger = Mapper.GetKeyPressing(Binding.Action);
			break;
		case ETriggerEvent::Completed:
			bShouldTrigger = Mapper.GetKeyRelease(Binding.Action);
			break;
		}

		if (bShouldTrigger && Binding.Callback) {
			// コールバック関数の実行（入力値を渡す）
			FInputActionValue Value;
			Value.bIsPressed = bShouldTrigger;
			if (Binding.Action == E_INPUT_ACTION::MOVE) {
				if (Mapper.GetKeyPressing(E_INPUT_ACTION::UP))    Value.Axis2D.Y += 1.0f;
				if (Mapper.GetKeyPressing(E_INPUT_ACTION::DOWN))  Value.Axis2D.Y -= 1.0f;
				if (Mapper.GetKeyPressing(E_INPUT_ACTION::LEFT))  Value.Axis2D.X += 1.0f;
				if (Mapper.GetKeyPressing(E_INPUT_ACTION::RIGHT)) Value.Axis2D.X -= 1.0f;
			}
			Binding.Callback(Value);
		}
	}
}
