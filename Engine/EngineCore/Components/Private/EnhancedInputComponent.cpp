#include "EnhancedInputComponent.h"

void MEnhancedInputComponent::ProcessInputBindings(const InputMapper& mapper) {
    for (const auto& b : m_bindings) {
        bool trigger = false;
        switch (b.Event) {
        case ETriggerEvent::Started:   trigger = mapper.GetPressStart(b.ActionName); break;
        case ETriggerEvent::Triggered: trigger = mapper.GetPressing(b.ActionName); break;
        case ETriggerEvent::Completed: trigger = mapper.GetRelease(b.ActionName); break;
        }
        if (!trigger || !b.Callback) continue;

        FInputActionValue value;
        value.bIsPressed = true;
        value.Axis2D.X = mapper.GetAxisValue(InputAction::MoveX);
        value.Axis2D.Y = mapper.GetAxisValue(InputAction::MoveY);
        b.Callback(value);
    }
}
