#include "EasyShakeComponent.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace
{
	constexpr float ShakeFadePortion = 0.2f;
	constexpr float LocationChangeEpsilonSq = 1e-6f;

	float ComputeEnvelope(float elapsedSeconds, float durationSeconds, bool bFadeIn, bool bFadeOut)
	{
		if (durationSeconds <= 0.0f) return 0.0f;

		float envelope = 1.0f;

		if (bFadeIn) {
			float fadeInTime = std::min(durationSeconds * ShakeFadePortion, durationSeconds * 0.5f);
			if (fadeInTime > 0.0f) {
				envelope *= std::clamp(elapsedSeconds / fadeInTime, 0.0f, 1.0f);
			}
		}

		if (bFadeOut) {
			float fadeOutTime = std::min(durationSeconds * ShakeFadePortion, durationSeconds * 0.5f);
			if (fadeOutTime > 0.0f) {
				envelope *= std::clamp((durationSeconds - elapsedSeconds) / fadeOutTime, 0.0f, 1.0f);
			}
		}

		return envelope;
	}

	FVector2D RandomOffsetInRange(const FVector2D& strengthXY)
	{
		static thread_local std::mt19937 rng{ std::random_device{}() };

		const float absX = std::abs(strengthXY.X);
		const float absY = std::abs(strengthXY.Y);

		std::uniform_real_distribution<float> distX(-absX, absX);
		std::uniform_real_distribution<float> distY(-absY, absY);

		return { distX(rng), distY(rng) };
	}
}

void MEasyShakeComponent::ApplyShake(const FVector2D& StrengthXY, float DurationSeconds, bool bEnableFadeIn, bool bEnableFadeOut)
{
	if (DurationSeconds <= 0.0f) return;

	FShakeInstance inst;
	inst.StrengthXY = StrengthXY;
	inst.DurationSeconds = DurationSeconds;
	inst.ElapsedSeconds = 0.0f;
	inst.bFadeIn = bEnableFadeIn;
	inst.bFadeOut = bEnableFadeOut;
	m_activeShakes.push_back(inst);
}

void MEasyShakeComponent::OnUpdate(float DeltaTime)
{
	MSceneComponent* target = GetParentComponent();
	if (!target) {
		m_activeShakes.clear();
		m_lastAppliedWorldOffset = FVector2D::ZeroVector;
		m_lastAppliedWorldLocation = FVector2D::ZeroVector;
		m_hasLastAppliedWorldLocation = false;
		return;
	}

	const FVector2D currentWorldLocation = target->GetWorldLocation();
	if (m_hasLastAppliedWorldLocation) {
		const float diffX = currentWorldLocation.X - m_lastAppliedWorldLocation.X;
		const float diffY = currentWorldLocation.Y - m_lastAppliedWorldLocation.Y;
		if ((diffX * diffX + diffY * diffY) > LocationChangeEpsilonSq) {
			m_lastAppliedWorldOffset = FVector2D::ZeroVector;
		}
	}

	// Advance & purge expired shakes, then compute the combined offset.
	FVector2D totalOffset = FVector2D::ZeroVector;
	for (auto it = m_activeShakes.begin(); it != m_activeShakes.end(); /* no increment */) {
		it->ElapsedSeconds += DeltaTime;

		if (it->ElapsedSeconds >= it->DurationSeconds) {
			it = m_activeShakes.erase(it);
			continue;
		}

		const float envelope = ComputeEnvelope(it->ElapsedSeconds, it->DurationSeconds, it->bFadeIn, it->bFadeOut);
		FVector2D offset = RandomOffsetInRange(it->StrengthXY) * envelope;
		totalOffset = totalOffset + offset;
		++it;
	}

	const FVector2D delta = totalOffset + (m_lastAppliedWorldOffset * -1.0f);
	if (delta.SizeSquared() > 0.0f) {
		target->AddWorldOffset(delta);
	}

	m_lastAppliedWorldOffset = totalOffset;
	m_lastAppliedWorldLocation = target->GetWorldLocation();
	m_hasLastAppliedWorldLocation = true;
}
