#pragma once
#include <string>
#include<vector>
#include "ActorComponent.h"
#include "UMath.h"
class AActor;

class MSceneComponent : public MActorComponent {
public:
	MSceneComponent();
	virtual ~MSceneComponent();
	virtual void OnUpdate(float DeltaTime);
    virtual void Draw();

	virtual void OnMessage(const std::string& message);

	void SetParentComponent(MSceneComponent* parent);
	auto GetParentComponent() const { return m_parentComponent; }

	bool SetWorldLocation(const FVector2D& NewWorldLocation);
	bool SetRelativeLocation(const FVector2D& NewRelativeLocation);
	bool AddWorldOffset(const FVector2D& Offset);
	bool AddLocalOffset(const FVector2D& Offset);
	FVector2D GetWorldLocation() const;
	FVector2D GetRelativeLocation() const;

	bool SetWorldRotation(FRotator NewRotation);
	bool AddWorldRotation(FRotator DeltaRotation);
	FRotator GetWorldRotation() const;
	FRotator GetRelativeRotation() const;

	bool SetRelativeScale(FScale NewScale);
	FScale GetRelativeScale() const;

	bool SetWorldScale(FScale NewScale);
	FScale GetWorldScale() const;

	void SetVisibility(bool bNewVisibility);
	bool IsVisible() const { return bVisible; }
	bool bVisible = true;

	bool bGridDirty() { return IsGridDirty; }
	void SetGridClean() { IsGridDirty = true; }

protected:
	void OnComponentDestroy() override;
	
	MSceneComponent* m_parentComponent = nullptr;
	std::vector<MSceneComponent*> m_childComponents;
	
	FVector2D RelativeLocation;
	FRotator RelativeRotation;
	FScale RelativeScale;

	mutable FVector2D WorldLocation;
	mutable FRotator WorldRotation;
	mutable FScale WorldScale;

	void MakeTransformDirty();
	void UpdateTransform() const;

	mutable bool IsTransformDirty = true;

	mutable bool IsGridDirty = true;

};
