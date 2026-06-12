#pragma once
#include "ActorComponent.h"
#include "InputMapper.h"
#include "UMath.h"
#include <functional>
#include <vector>
#include <string>

enum class ETriggerEvent { Started, Triggered, Completed };

struct FInputActionValue {
	FVector2D Axis2D = FVector2D::ZeroVector;
	float Axis1D = 0.0f;
	bool      bIsPressed = false;
};

class MEnhancedInputComponent : public MActorComponent {
private:
	struct FInputBinding {
		std::string  ActionName;
		ETriggerEvent Event;
		std::function<void(const FInputActionValue&)> Callback;
	};
	std::vector<FInputBinding> m_bindings;

public:
	void ClearBindings() { m_bindings.clear(); }
	template<class T>
	void BindAction(const std::string& actionName, ETriggerEvent event,
		T* obj, void (T::* func)(const FInputActionValue&)) {
		m_bindings.push_back({ actionName, event,
			[obj, func](const FInputActionValue& v) { (obj->*func)(v); } });
	}
	template<class T>
	void BindAction(const std::string& actionName, ETriggerEvent event,
		T* obj, void (T::* func)()) {
		m_bindings.push_back({ actionName, event,
			[obj, func](const FInputActionValue&) { (obj->*func)(); } });
	}

	void ProcessInputBindings(const InputMapper& mapper);
};
