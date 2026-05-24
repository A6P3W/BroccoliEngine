#include "EnhancedInputComponent.h"

void MEnhancedInputComponent::ProcessInputBindings(const InputMapper& mapper) {
    for (const auto& b : m_bindings) {
        float axisVal = mapper.GetAxisValue(b.ActionName);
        bool trigger = false;
        switch (b.Event) {
        case ETriggerEvent::Started:   trigger = mapper.GetPressStart(b.ActionName); break;
        case ETriggerEvent::Triggered: trigger = mapper.GetPressing(b.ActionName) || (std::abs(axisVal) > 0.0001f); break;
        case ETriggerEvent::Completed: trigger = mapper.GetRelease(b.ActionName); break;
        }
        if (!trigger || !b.Callback) continue;

        FInputActionValue value;
        value.bIsPressed = true;
        value.Axis1D = axisVal;

        value.Axis2D.X = mapper.GetAxisValue(InputAction::MoveX);
        value.Axis2D.Y = mapper.GetAxisValue(InputAction::MoveY);

        b.Callback(value);
    }
}
