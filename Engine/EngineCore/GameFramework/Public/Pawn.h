#pragma once
#include "Actor.h"
#include "UMath.h"

class MCameraComponent;
class MEnhancedInputComponent;
struct FInputActionValue;
class APawn : public AActor
{
public:
	DEFINE_ACTOR_CLASS(APawn);
	APawn();
	virtual void OnPossesed();
    void OnUpdate(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent);

private:
	void OnInteractPressed();
	virtual void OnMove(const FInputActionValue& Value);
protected:
	MCameraComponent* m_camera = nullptr;
	FVector2D ControlInputVector = {0,0};
};

