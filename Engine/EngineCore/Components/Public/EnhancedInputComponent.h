#pragma once
#include "ActorComponent.h"
#include "InputMapper.h"
#include "UMath.h"
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>

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
		bool IsUIAction = false;
	};
	std::vector<FInputBinding> m_bindings;
	std::unordered_map<std::string, FVector2D> m_lastAxis2DValues;
	std::unordered_map<std::string, float> m_lastAxis1DValues;

public:
	void ClearBindings() {
		m_bindings.clear();
		m_lastAxis2DValues.clear();
		m_lastAxis1DValues.clear();
	}
	template<class T>
	void BindAction(const std::string& actionName, ETriggerEvent event,
		T* obj, void (T::* func)(const FInputActionValue&), bool isUIAction = false) {
		m_bindings.push_back({ actionName, event,
			[obj, func](const FInputActionValue& v) { (obj->*func)(v); }, isUIAction });
	}
	template<class T>
	void BindAction(const std::string& actionName, ETriggerEvent event,
		T* obj, void (T::* func)(), bool isUIAction = false) {
		m_bindings.push_back({ actionName, event,
			[obj, func](const FInputActionValue&) { (obj->*func)(); }, isUIAction });
	}

	void ProcessInputBindings(const InputMapper& mapper, bool AllowUI, bool AllowGame);
};
