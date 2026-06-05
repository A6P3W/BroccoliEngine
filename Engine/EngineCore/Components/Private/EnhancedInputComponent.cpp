#include "EnhancedInputComponent.h"

void MEnhancedInputComponent::ProcessInputBindings(const InputMapper& mapper) {
    for (const auto& b : m_bindings) {
        float axisVal = mapper.GetAxisValue(b.ActionName);
        bool trigger = false;
        switch (b.Event) {
        case ETriggerEvent::Started:   trigger = mapper.GetPressStart(b.ActionName); break;
        case ETriggerEvent::Triggered:
            if (b.ActionName == InputAction::Move) {
                FVector2D moveAxis2D = mapper.GetAxis2DValue(InputActionLower::MoveX, InputActionLower::MoveY);
                trigger = (moveAxis2D.SizeSquared() > 0.0001f);
            }
            else if (b.ActionName == InputAction::Look) {
                FVector2D lookAxis2D = mapper.GetAxis2DValue(InputActionLower::LookX, InputActionLower::LookY);
                trigger = (lookAxis2D.SizeSquared() > 0.0001f);
            }
            else{ 
                trigger = mapper.GetPressing(b.ActionName) || (std::abs(axisVal) > 0.0001f); }
            break;
        case ETriggerEvent::Completed: trigger = mapper.GetRelease(b.ActionName); break;
        }
        if (!trigger || !b.Callback) continue;

        FInputActionValue value;
        value.bIsPressed = true;
        value.Axis1D = axisVal;



        if (b.ActionName == InputAction::Move) {
            value.Axis2D = mapper.GetAxis2DValue(InputActionLower::MoveX, InputActionLower::MoveY);
        }
		else if (b.ActionName == InputAction::Look) {
			value.Axis2D = mapper.GetAxis2DValue(InputActionLower::LookX, InputActionLower::LookY);
		}
        b.Callback(value);
    }
}
