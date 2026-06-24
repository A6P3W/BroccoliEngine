#pragma once
#include "BaseObject.h"
#include <string>
#include <string_view>
#include <vector>
#include "SceneComponent.h"
#include "UMath.h"
#include "ActorComponent.h"
#include "ActorRegistry.h"
#include "NetBuffer.h"
#include "NetworkTypes.h"
#include <cstdint>

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
class MTimerManager;
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
	FNetworkActorId NetworkId = 0;
	bool bReplicates = false;

	bool bHasAuthority = true;
	bool bIsLocallyControlled = false;
	FNetworkConnectionId OwnerConnectionId = 0;
	bool HasTag(std::string_view Tag) const;

	virtual void Update(float DeltaTime) final;
	virtual void Draw();
	MSceneComponent* GetRootComponent() const { return RootComponent; };
	MTimerManager& GetWorldTimerManager();

	const std::vector<std::unique_ptr<MActorComponent>>& GetComponents() const;

	void AddComponent(std::unique_ptr<MActorComponent> comp);

	FVector2D GetActorLocation() const;
	bool SetActorLocation(const FVector2D& NewLocation);

	FRotator GetActorRotation() const;
	bool SetActorRotation(const FRotator& NewRotation);

	FScale GetActorScale() const;
	bool SetActorScale(FScale NewScale);


	void AddActorWorldOffset(const FVector2D& Offset);
	void AddActorLocalOffset(const FVector2D& Offset);
	void AddActorRotation(const FRotator& DeltaRotation);

	void Destroy();
	bool IsPendingDestroy() const;

	virtual void BeginOverlap(AActor* OtherActor) {}
	virtual void EndOverlap(AActor* OtherActor) {}

	virtual void SerializeNetworkState(FNetBuffer& OutBuffer);
	virtual bool DeserializeNetworkState(FNetBuffer& InBuffer);

	virtual void SerializeNetworkSpawn(FNetBuffer& OutBuffer);
	virtual bool DeserializeNetworkSpawn(FNetBuffer& InBuffer);

	bool HasReplicatedStateChanged(float Tolerance = 0.001f) const;
	void UpdateReplicatedStateCache();
	uint32_t IncrementReplicationSequence();
	uint32_t GetReplicationSequence() const { return ReplicationSequence; }
	uint32_t GetLastReceivedReplicationSequence() const { return LastReceivedReplicationSequence; }
	void SetLastReceivedReplicationSequence(uint32_t Sequence);

	template<class T>
	std::vector <T*> GetComponents() const {
		std::vector<T*> results;
		for (const auto& comp : Components) {
			if (auto casted = dynamic_cast<T*>(comp.get())) {
				results.push_back(casted);
			}
		}
		return results;
	}
	World* GetWorld() { return OwnerWorld; }
	void SetWorld(World* world);

	bool IsEditorActor() const { return bEditorActor; }
	bool CanUpdateAnytime() const { return bUpdateableAnytime; }
	void SetUpdateableAnytime(bool bTick) { bUpdateableAnytime = bTick; }
protected:
	virtual void BeginPlay() {}
	virtual void OnUpdate(float DeltaTime);
	MSceneComponent* RootComponent = nullptr;
	void SetRootComponent(MSceneComponent* Component);

	bool bEditorActor = false;
	bool bUpdateableAnytime = false;

private:
	std::vector<AActor*> ChildObjects;
	bool bPendingDestroy = false;
	std::vector<std::unique_ptr<MActorComponent>> Components;
	World* OwnerWorld=nullptr;
	FVector2D LastReplicatedLocation = FVector2D::ZeroVector;
	FRotator LastReplicatedRotation;
	FScale LastReplicatedScale;
	bool bHasReplicatedStateCache = false;
	uint32_t ReplicationSequence = 0;
	uint32_t LastReceivedReplicationSequence = 0;

};
