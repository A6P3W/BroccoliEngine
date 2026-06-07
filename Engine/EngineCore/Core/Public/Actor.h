#pragma once
#include "BaseObject.h"
#include <string>
#include <string_view>
#include <vector>
#include "SceneComponent.h"
#include "Utils/Umath.h"
#include "ActorComponent.h"
#include "ActorRegistry.h"

template<class T>
struct TActorAutoRegister
{
	TActorAutoRegister()
	{
		ActorRegistry::GetInstance().Register<T>();
	}
};

#define REGISTER_ACTOR(ClassName) \
    static TActorAutoRegister<ClassName> AutoRegister_##ClassName;


#define DEFINE_ACTOR_CLASS(ClassName) \
public: \
	static std::string StaticClassName() { return #ClassName; }\
    virtual std::string GetActorClassName() const override { return #ClassName; }


class MSceneComponent;
class TimerManager;
class World;
class AActor :
	public MBaseObject

{

public:

	AActor();
	virtual ~AActor() override;

	void Spawned();

	virtual std::string GetActorClassName() const = 0;

	std::vector<std::string> Tags;
	bool HasTag(std::string_view Tag) const;

	virtual void Update(float DeltaTime) final;
	virtual void Draw();
	MSceneComponent* GetRootComponent() const { return m_rootComponent; };
	TimerManager& GetWorldTimerManager();

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
	World* GetWorld() { return m_world; }
	void SetWorld(World* world);

	bool IsEditorActor() const { return bEditorActor; }
protected:
	virtual void BeginPlay() {}
	virtual void OnUpdate(float DeltaTime);
	MSceneComponent* m_rootComponent = nullptr;
	void SetRootComponent(MSceneComponent* Component);

	bool bEditorActor = false;

private:
	std::vector<AActor*> m_childObjects;
	bool m_PendingDestroy = false;
	std::vector<std::unique_ptr<MActorComponent>> m_components;
	World* m_world=nullptr;

	
};
