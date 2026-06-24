#include "NetworkManager.h"

#include "NetPacketType.h"

#include <enet/enet.h>

#include <unordered_map>
#include <vector>

struct NetworkManager::Impl
{
	std::unordered_map<FNetworkConnectionId, ENetPeer*> PeersByConnectionId;
	std::unordered_map<ENetPeer*, FNetworkConnectionId> ConnectionIdsByPeer;
};

NetworkManager& NetworkManager::GetInstance()
{
	static NetworkManager Instance;
	return Instance;
}

NetworkManager::NetworkManager()
	: ImplPtr(new Impl())
{
	bEnetInitialized = (enet_initialize() == 0);
}

NetworkManager::~NetworkManager()
{
	Stop();
	delete ImplPtr;
	ImplPtr = nullptr;
	if (bEnetInitialized) {
		enet_deinitialize();
	}
}

bool NetworkManager::StartServer(uint16_t Port, size_t MaxConnections, size_t ChannelCount)
{
	Stop();
	if (!bEnetInitialized) {
		return false;
	}

	ENetAddress address{};
	address.host = ENET_HOST_ANY;
	address.port = Port;

	Host = enet_host_create(
		&address,
		MaxConnections,
		ChannelCount,
		0,
		0);

	if (!Host) {
		return false;
	}

	bIsServer = true;
	bIsClient = false;
	NextConnectionId = 1;
	LocalConnectionId = 0;
	return true;
}

bool NetworkManager::ConnectToServer(const std::string& HostName, uint16_t Port, size_t ChannelCount)
{
	Stop();
	if (!bEnetInitialized) {
		return false;
	}

	Host = enet_host_create(
		nullptr,
		1,
		ChannelCount,
		0,
		0);

	if (!Host) {
		return false;
	}

	ENetAddress address{};
	if (enet_address_set_host(&address, HostName.c_str()) != 0) {
		Stop();
		return false;
	}
	address.port = Port;

	ServerPeer = enet_host_connect(Host, &address, ChannelCount, 0);
	if (!ServerPeer) {
		Stop();
		return false;
	}

	bIsServer = false;
	bIsClient = true;
	LocalConnectionId = 0;
	return true;
}

void NetworkManager::Service()
{
	if (!Host) {
		return;
	}

	ENetEvent event{};
	while (enet_host_service(Host, &event, 0) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
		{
			const FNetworkConnectionId connectionId = RegisterPeer(event.peer);
			if (OnConnected) {
				OnConnected(connectionId);
			}
			break;
		}
		case ENET_EVENT_TYPE_DISCONNECT:
		{
			const FNetworkConnectionId connectionId = FindConnectionId(event.peer);
			RemovePeer(event.peer);
			if (OnDisconnected && connectionId != 0) {
				OnDisconnected(connectionId);
			}
			break;
		}
		case ENET_EVENT_TYPE_RECEIVE:
		{
			const FNetworkConnectionId connectionId = FindConnectionId(event.peer);
			if (connectionId != 0 && event.packet && event.packet->data && event.packet->dataLength > 0) {
				std::vector<uint8_t> bytes(event.packet->data, event.packet->data + event.packet->dataLength);
				FNetBuffer buffer(std::move(bytes));

				ENetPacketType packetType = ENetPacketType::None;
				if (buffer.Read(packetType)) {
					buffer.ResetRead();
					if (OnPacketReceived) {
						OnPacketReceived(connectionId, buffer);
					}
				}
			}

			if (event.packet) {
				enet_packet_destroy(event.packet);
			}
			break;
		}
		default:
			break;
		}
	}
}

void NetworkManager::Disconnect()
{
	if (!Host) {
		return;
	}

	if (bIsClient && ServerPeer) {
		enet_peer_disconnect(ServerPeer, 0);
	}
	else {
		for (const auto& pair : ImplPtr->PeersByConnectionId) {
			if (pair.second) {
				enet_peer_disconnect(pair.second, 0);
			}
		}
	}

	enet_host_flush(Host);
}

void NetworkManager::Stop()
{
	if (!Host) {
		ClearPeers();
		return;
	}

	if (bIsClient && ServerPeer) {
		enet_peer_disconnect_now(ServerPeer, 0);
	}
	else {
		for (const auto& pair : ImplPtr->PeersByConnectionId) {
			if (pair.second) {
				enet_peer_disconnect_now(pair.second, 0);
			}
		}
	}

	enet_host_destroy(Host);
	Host = nullptr;
	ServerPeer = nullptr;
	bIsServer = false;
	bIsClient = false;
	NextConnectionId = 1;
	LocalConnectionId = 0;
	ClearPeers();
}

bool NetworkManager::SendToServer(const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId)
{
	if (!bIsClient || !ServerPeer) {
		return false;
	}

	return SendToPeer(ServerPeer, Buffer, Reliability, ChannelId);
}

bool NetworkManager::SendToClient(FNetworkConnectionId ConnectionId, const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId)
{
	if (!bIsServer) {
		return false;
	}

	auto it = ImplPtr->PeersByConnectionId.find(ConnectionId);
	if (it == ImplPtr->PeersByConnectionId.end()) {
		return false;
	}

	return SendToPeer(it->second, Buffer, Reliability, ChannelId);
}

bool NetworkManager::Broadcast(const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId)
{
	if (!Host || !bIsServer || Buffer.Size() == 0) {
		return false;
	}

	const enet_uint32 flags = (Reliability == ENetPacketReliability::Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0;
	ENetPacket* packet = enet_packet_create(Buffer.Data(), Buffer.Size(), flags);
	if (!packet) {
		return false;
	}

	enet_host_broadcast(Host, ChannelId, packet);
	enet_host_flush(Host);
	return true;
}

bool NetworkManager::SendToPeer(ENetPeer* Peer, const FNetBuffer& Buffer, ENetPacketReliability Reliability, uint8_t ChannelId)
{
	if (!Host || !Peer || Buffer.Size() == 0) {
		return false;
	}

	const enet_uint32 flags = (Reliability == ENetPacketReliability::Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0;
	ENetPacket* packet = enet_packet_create(Buffer.Data(), Buffer.Size(), flags);
	if (!packet) {
		return false;
	}

	if (enet_peer_send(Peer, ChannelId, packet) != 0) {
		enet_packet_destroy(packet);
		return false;
	}

	enet_host_flush(Host);
	return true;
}

FNetworkConnectionId NetworkManager::RegisterPeer(ENetPeer* Peer)
{
	if (!Peer) {
		return 0;
	}

	const FNetworkConnectionId existingId = FindConnectionId(Peer);
	if (existingId != 0) {
		return existingId;
	}

	const FNetworkConnectionId connectionId = bIsClient ? ServerConnectionId : NextConnectionId++;
	ImplPtr->PeersByConnectionId[connectionId] = Peer;
	ImplPtr->ConnectionIdsByPeer[Peer] = connectionId;
	if (bIsClient) {
		ServerPeer = Peer;
		LocalConnectionId = connectionId;
	}

	return connectionId;
}

FNetworkConnectionId NetworkManager::FindConnectionId(ENetPeer* Peer) const
{
	if (!Peer) {
		return 0;
	}

	auto it = ImplPtr->ConnectionIdsByPeer.find(Peer);
	if (it == ImplPtr->ConnectionIdsByPeer.end()) {
		return 0;
	}

	return it->second;
}

void NetworkManager::RemovePeer(ENetPeer* Peer)
{
	const FNetworkConnectionId connectionId = FindConnectionId(Peer);
	if (connectionId != 0) {
		ImplPtr->PeersByConnectionId.erase(connectionId);
	}
	ImplPtr->ConnectionIdsByPeer.erase(Peer);

	if (Peer == ServerPeer) {
		ServerPeer = nullptr;
	}
}

void NetworkManager::ClearPeers()
{
	if (!ImplPtr) {
		return;
	}

	ImplPtr->PeersByConnectionId.clear();
	ImplPtr->ConnectionIdsByPeer.clear();
}
