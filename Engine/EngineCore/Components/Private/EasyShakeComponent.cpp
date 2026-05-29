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
	inst.Id = 0; // non-handle shake
	inst.StrengthXY = StrengthXY;
	inst.DurationSeconds = DurationSeconds;
	inst.ElapsedSeconds = 0.0f;
	inst.bFadeIn = bEnableFadeIn;
	inst.bFadeOut = bEnableFadeOut;
	m_activeShakes.push_back(inst);
}

void MEasyShakeComponent::StartShake(FShakeHandle& Handle, const FVector2D& StrengthXY, float DurationSeconds)
{
	if (DurationSeconds <= 0.0f) {
		Handle.Invalidate();
		return;
	}

	// If handle already refers to an active shake, end it first (no fade-out)
	if (Handle.IsValid()) {
		EndShake(Handle);
	}

	uint64_t NewId = ++m_nextShakeId;
	Handle = FShakeHandle(NewId);

	FShakeInstance inst;
	inst.Id = Handle.GetId();
	inst.StrengthXY = StrengthXY;
	inst.DurationSeconds = DurationSeconds;
	inst.ElapsedSeconds = 0.0f;
	inst.bFadeIn = false;
	inst.bFadeOut = false;
	m_activeShakes.push_back(inst);
}

void MEasyShakeComponent::EndShake(FShakeHandle& Handle, bool bEnableFadeOut)
{
	if (!Handle.IsValid()) return;

	const uint64_t Id = Handle.GetId();
	for (auto it = m_activeShakes.begin(); it != m_activeShakes.end(); ++it) {
		if (it->Id == Id) {
			if (!bEnableFadeOut) {
				m_activeShakes.erase(it);
				Handle.Invalidate();
			}
			else {
				// enable fade-out by shortening duration to a small fade window
				float originalDuration = it->DurationSeconds;
				float fadeOutTime = std::min(originalDuration * ::ShakeFadePortion, originalDuration * 0.5f);
				it->bFadeOut = true;
				it->DurationSeconds = it->ElapsedSeconds + fadeOutTime;
			}
			break;
		}
	}
}

bool MEasyShakeComponent::IsShakeActive(const FShakeHandle& Handle) const
{
	if (!Handle.IsValid()) return false;
	const uint64_t Id = Handle.GetId();
	for (const auto& inst : m_activeShakes) {
		if (inst.Id == Id) return true;
	}
	return false;
}

void MEasyShakeComponent::OnUpdate(float DeltaTime)
{
	MSceneComponent* target = this;
	if (!target) return;

	if (m_lastAppliedLocalOffset.SizeSquared() > 1e-8f) {
		target->AddLocalOffset(m_lastAppliedLocalOffset * -1.0f);
		m_lastAppliedLocalOffset = FVector2D::ZeroVector;
	}

	if (m_activeShakes.empty()) {
		return;
	}

	FVector2D totalOffset = FVector2D::ZeroVector;
	for (auto it = m_activeShakes.begin(); it != m_activeShakes.end();) {
		it->ElapsedSeconds += DeltaTime;
		if (it->ElapsedSeconds >= it->DurationSeconds) {
			it = m_activeShakes.erase(it);
			continue;
		}

		const float envelope = ComputeEnvelope(it->ElapsedSeconds, it->DurationSeconds, it->bFadeIn, it->bFadeOut);
		totalOffset = totalOffset + RandomOffsetInRange(it->StrengthXY) * envelope;
		++it;
	}


	if (totalOffset.SizeSquared() > 1e-8f) {
		target->AddLocalOffset(totalOffset);
		m_lastAppliedLocalOffset = totalOffset;
	}
}
