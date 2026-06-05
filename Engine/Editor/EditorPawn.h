#pragma once
#include "Pawn.h"
#include <Utils/Umath.h>

class MEnhancedInputComponent;
struct FInputActionValue;
class MMovementComponent;
class EditorMode;
class EditorPawn : public APawn
{
public:
	DEFINE_ACTOR_CLASS(EditorPawn);
    EditorPawn();
    
    void OnUpdate(float DeltaTime) override;
    void OnPossesed() override;
    void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;
private:
	void BeginPlay() override;
    void OnMouseLeftPress(const FInputActionValue& Value);
    void OnMouseLeftRelease(const FInputActionValue& Value);

	void OnMouseRightPress(const FInputActionValue& Value);
	void OnMouseRightRelease(const FInputActionValue& Value);
	void OnMouseMove(const FInputActionValue& Value);
    FVector2D GetMouseWorldPosition() const;

	EditorMode* m_editorMode = nullptr;

    bool m_RightMousePressed = false;
    FVector2D m_dragStartMousePos; 
    FVector2D m_dragStartCameraPos;
};
