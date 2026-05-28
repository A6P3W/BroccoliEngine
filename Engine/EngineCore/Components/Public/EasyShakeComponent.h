#pragma once
#include "Components/Public/SceneComponent.h"
#include <vector>

class MEasyShakeComponent : public MSceneComponent
{
public:
	void ApplyShake(const FVector2D& StrengthXY, float DurationSeconds, bool bEnableFadeIn, bool bEnableFadeOut);

	void OnUpdate(float DeltaTime) override;

private:
	struct FShakeInstance
	{
		FVector2D StrengthXY = FVector2D::ZeroVector;
		float DurationSeconds = 0.0f;
		float ElapsedSeconds = 0.0f;
		bool bFadeIn = false;
		bool bFadeOut = false;
	};

	std::vector<FShakeInstance> m_activeShakes;
	FVector2D m_lastAppliedWorldOffset = FVector2D::ZeroVector;
	FVector2D m_lastAppliedWorldLocation = FVector2D::ZeroVector;
	bool m_hasLastAppliedWorldLocation = false;
};
