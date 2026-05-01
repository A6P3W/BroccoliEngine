#pragma once
#include <string>
#include "UpdateableComponent.h"
#include "Utils/UMath.h"
class MUpdateableObject; // 前方宣言

class MSceneComponent : public UpdateableComponent {
public:
	MSceneComponent();
	virtual ~MSceneComponent();
	virtual void OnUpdate(float DeltaTime);
	void Draw() override;

	// メッセージ受信用の仮想関数
	virtual void OnMessage(const std::string& message);

	void SetOwner(MUpdateableObject* owner);

	void SetLocation(const FVector2D& nPos);
	void AddLocalLocation(float nx, float ny);
	void AddWorldLocation(float nx, float ny);
	const FVector2D& GetLocation() const;

	void SetWorldRotation(float nAngle);
	void AddRotation(float nAngleDeg);
	float GetWorldRotation() const;

	void SetScale(float nScale);
	float GetScale() const;

protected:
	MUpdateableObject* m_owner = nullptr;
	FVector2D Location;
	FRotator Rotation;
	float Scale = 1.0f;
};