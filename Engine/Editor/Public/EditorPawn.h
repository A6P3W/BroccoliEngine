#pragma once
#include "Pawn.h"
#include "UMath.h"

class MEnhancedInputComponent;
struct FInputActionValue;
class MMovementComponent;
class EditorMode;
class MSpriteComponent;
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

	void OnMove(const FInputActionValue& Value);

    void OnMouseLeftPress(const FInputActionValue& Value);
    void OnMouseLeftRelease(const FInputActionValue& Value);

	void OnMouseRightPress(const FInputActionValue& Value);
	void OnMouseRightRelease(const FInputActionValue& Value);
	void OnMouseMove(const FInputActionValue& Value);
	void OnWheel(const FInputActionValue& Value);
    FVector2D GetMouseWorldPosition() const;

	EditorMode* m_editorMode = nullptr;

    bool m_RightMousePressed = false;
    FVector2D m_dragStartMousePos; 
    FVector2D m_dragStartCameraPos;

	int MousePointX, MousePointY=0;
    MSpriteComponent* GameScreenView;
};
