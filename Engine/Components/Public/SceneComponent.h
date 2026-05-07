#pragma once
#include <string>
#include<vector>
#include "ActorComponent.h"
#include "Utils/UMath.h"
class MActorComponent; 
class AActor;

class MSceneComponent : public MActorComponent {
public:
	MSceneComponent();
	virtual ~MSceneComponent();
	virtual void OnUpdate(float DeltaTime);
    virtual void Draw();

	// メッセージ受信用の仮想関数
	virtual void OnMessage(const std::string& message);

	void SetOwner(AActor* owner);
	auto GetOwner() const { return m_owner; }

	void SetParentComponent(MSceneComponent* parent);
	auto GetParentComponent() const { return m_parentComponent; }

	bool SetWorldLocation(const FVector2D& NewWorldLocation);
	bool SetRelativeLocation(const FVector2D& NewRelativeLocation);
	bool AddWorldOffset(const FVector2D& Offset);
	bool AddLocalOffset(const FVector2D& Offset);
	FVector2D GetWorldLocation() const;
	FVector2D GetRelativeLocation() const;

	bool SetWorldRotation(float nAngle);
	bool AddWorldRotation(float nAngleDeg);
	FRotator GetWorldRotation() const;
	FRotator GetRelativeRotation() const;

	bool SetScale(float nScale);
	float GetScale() const;

	FVector2D RelativeLocation;
	FRotator RelativeRotation;
	FScale Scale;
protected:
	AActor* m_owner = nullptr;
	MSceneComponent* m_parentComponent = nullptr;
	std::vector<MSceneComponent*> m_childComponents;
	
};
