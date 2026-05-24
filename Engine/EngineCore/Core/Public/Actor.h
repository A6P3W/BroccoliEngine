#pragma once
#include "BaseObject.h"
#include <string>
#include <vector>
#include "SceneComponent.h"
#include "Utils/Umath.h"
#include "ActorComponent.h"

#define DEFINE_ACTOR_CLASS(ClassName) \
public: \
    virtual std::string GetActorClassName() const override { return #ClassName; }\


class MSceneComponent;
class AActor :
	public MBaseObject

{

public:

	AActor();

	virtual std::string GetActorClassName() const = 0;


	virtual void Update(float DeltaTime) final;
	virtual void Draw()final;
	MSceneComponent* GetRootComponent() const { return m_rootComponent; };

	const std::vector<std::unique_ptr<MActorComponent>>& GetComponents() const;

	void AddComponent(std::unique_ptr<MActorComponent> comp);

	FVector2D GetActorLocation() const;
	bool SetActorLocation(const FVector2D& NewLocation);

	FRotator GetActorRotation() const;
	bool SetActorRotation(const FRotator& NewRotation);

	FScale GetActorScale() const;
	bool SetActorScale(float NewScale);


	void AddActorWorldOffset(const FVector2D& Offset);
	void AddActorLocalOffset(const FVector2D& Offset);
	void AddActorRotation(const FRotator& DeltaRotation);

	void Destroy();
	bool IsPendingDestroy() const;

	virtual void BeginOverlap(AActor* OtherActor) {}
	virtual void EndOverlap(AActor* OtherActor) {}

	template<class T>
	std::vector <T*> GetComponents() const {
		std::vector<T*> results;
		for (const auto& comp : m_components) {
			if (auto casted = dynamic_cast<T*>(comp.get())) {
				results.push_back(casted);
			}
		}
		return results;
	}
protected:
	virtual void OnUpdate(float DeltaTime);
	MSceneComponent* m_rootComponent = nullptr;
	void SetRootComponent(MSceneComponent* Component);
private:
	std::vector<AActor*> m_childObjects;
	bool m_PendingDestroy = false;
	std::vector<std::unique_ptr<MActorComponent>> m_components;
};

