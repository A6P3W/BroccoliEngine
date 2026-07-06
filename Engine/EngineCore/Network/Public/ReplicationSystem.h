#pragma once

#include <string>
#include <unordered_map>

#include "NetworkManager.h"
#include "NetworkTypes.h"

class AActor;
class FNetBuffer;
class World;

class FReplicationSystem {
 public:
  explicit FReplicationSystem(World* InWorld);
  ~FReplicationSystem();

  void Update();

  AActor* FindActor(FNetworkActorId NetworkId) const;
  void RegisterActor(AActor* Actor);
  void UnregisterActor(FNetworkActorId NetworkId);
  void Clear();
  bool SendActorRPC(
      AActor* Actor,
      FNetworkRPCId RPCId,
      ENetRPCType RPCType,
      ENetPacketReliability Reliability,
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

  World* OwnerWorld = nullptr;
  NetworkManager::CallbackHandle ConnectedCallbackHandle = 0;
  NetworkManager::CallbackHandle DisconnectedCallbackHandle = 0;
  NetworkManager::CallbackHandle PacketReceivedCallbackHandle = 0;
  std::unordered_map<FNetworkActorId, AActor*> ActorsByNetworkId;
  std::unordered_map<FNetworkConnectionId, std::string> LastReadyTravelByConnection;
};
