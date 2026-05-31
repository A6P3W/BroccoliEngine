#pragma once
#include "Pawn.h"
#include <Utils/Umath.h>

class MEnhancedInputComponent;
struct FInputActionValue;
class MMovementComponent;
class EditorPawn : public APawn
{
public:
	DEFINE_ACTOR_CLASS(EditorPawn);
    EditorPawn();
    void OnUpdate(float DeltaTime) override;
    void OnPossesed() override;
    void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;
private:
    void OnMouseLeftPress(const FInputActionValue& Value);
    void OnMouseLeftRelease(const FInputActionValue& Value);

    FVector2D GetMouseWorldPosition() const;
};
