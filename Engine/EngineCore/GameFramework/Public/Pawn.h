#pragma once
#include "Actor.h"
#include <Utils/Umath.h>

class MCameraComponent;
class MEnhancedInputComponent;
struct FInputActionValue;
class APawn : public AActor
{
public:
	APawn();
	virtual void OnPossesed();
    void OnUpdate(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent);

	MEnhancedInputComponent* GetInputComponent() { return m_InputCompPtr; }
private:
	void OnInteractPressed();
	virtual void OnMove(const FInputActionValue& Value);
protected:
	MCameraComponent* m_camera = nullptr;
	FVector2D ControlInputVector = {0,0};
	MEnhancedInputComponent* m_InputCompPtr;
};

