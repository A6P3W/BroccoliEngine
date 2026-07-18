#pragma once
#include <string>

#include "BroccoliEngineAPI.h"
#include "NetworkManager.h"
#include "NetworkTypes.h"

class AActor;
class FNetBuffer;
class World;

class BROCCOLI_ENGINE_API FReplicationSystem {
 public:
  explicit FReplicationSystem(World* InWorld);
  ~FReplicationSystem();
  FReplicationSystem(const FReplicationSystem&) = delete;
  FReplicationSystem& operator=(const FReplicationSystem&) = delete;

  void Update();

  AActor* FindActor(FNetworkActorId NetworkId) const;
  void RegisterActor(AActor* Actor);
  void UnregisterActor(FNetworkActorId NetworkId);
  void Clear();
  bool SendActorRPC(
      AActor* Actor,
      FNetworkRPCId RPCId,
      ENetRPCType RPCType,
      ENetworkReliability Reliability,
      const FNetBuffer& Payload,
      FNetworkComponentId ComponentNetworkId = 0
  );
  bool BroadcastServerTravel(FNetworkSceneId SceneId);
  bool BroadcastServerTravel(const std::string& LevelPath);
  void NotifySceneLoaded();

 private:
  void HandleConnected(FNetworkConnectionId ConnectionId);
  void HandleDisconnected(FNetworkConnectionId ConnectionId);
  void HandlePacketReceived(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer);

  void HandleActorSpawn(FNetBuffer& Buffer);
  void HandleActorState(FNetBuffer& Buffer);
  void HandleActorDestroy(FNetBuffer& Buffer);
  void HandleActorRPC(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer);
  void HandleAssignNetId(FNetBuffer& Buffer);
  void HandleServerTravel(FNetBuffer& Buffer);
  void HandleClientTravelReady(FNetworkConnectionId ConnectionId, FNetBuffer& Buffer);

  void SendAssignedConnectionId(FNetworkConnectionId ConnectionId);
  bool SendServerTravelToClient(FNetworkConnectionId ConnectionId, FNetworkSceneId SceneId);
  bool SendServerTravelToClient(FNetworkConnectionId ConnectionId, const std::string& LevelPath);
  bool SendClientTravelReady(FNetworkSceneId SceneId);
  bool SendClientTravelReady(const std::string& LevelPath);

  void SendInitialStateToClient(FNetworkConnectionId ConnectionId);
  bool SendActorSpawn(AActor* Actor, FNetworkConnectionId TargetConnectionId = 0);
  bool SendActorState(AActor* Actor);
  void SendActorDestroy(FNetworkActorId NetworkId);

  bool EnsureServerActorRegistered(AActor* Actor);

  void RefreshLocalControl();

  void InitializeClientForCurrentScene(FNetworkConnectionId ConnectionId);

  struct Impl;
  Impl* ImplPtr = nullptr;
};
