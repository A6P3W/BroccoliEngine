#include "NetworkUtils.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// clang-format off
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Iphlpapi.h>
// clang-format on

#include <array>
#include <vector>

#include "Log.h"

namespace {
std::string GetActiveWiFiIPv4() {
  ULONG BufferSize = 0;
  constexpr ULONG Flags =
      GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
  ULONG Result = GetAdaptersAddresses(AF_INET, Flags, nullptr, nullptr, &BufferSize);
  if (Result != ERROR_BUFFER_OVERFLOW || BufferSize == 0) {
    M_LOG("[NetworkUtils] Adapter size lookup failed: result={} bufferSize={}", Result, BufferSize);
    return {};
  }

  std::vector<unsigned char> Buffer(BufferSize);
  auto* Addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(Buffer.data());
  Result = GetAdaptersAddresses(AF_INET, Flags, nullptr, Addresses, &BufferSize);
  if (Result != NO_ERROR) {
    M_LOG("[NetworkUtils] Adapter lookup failed: result={}", Result);
    return {};
  }

  std::string WiFiAddress;
  for (IP_ADAPTER_ADDRESSES* Adapter = Addresses; Adapter; Adapter = Adapter->Next) {
    const bool IsWiFi = Adapter->IfType == IF_TYPE_IEEE80211;
    const bool IsUp = Adapter->OperStatus == IfOperStatusUp;
    const bool HasGateway = Adapter->FirstGatewayAddress != nullptr;
    bool LoggedAddress = false;

    for (IP_ADAPTER_UNICAST_ADDRESS* Unicast = Adapter->FirstUnicastAddress; Unicast;
         Unicast = Unicast->Next) {
      if (!Unicast->Address.lpSockaddr || Unicast->Address.lpSockaddr->sa_family != AF_INET) {
        continue;
      }

      const auto* SocketAddress = reinterpret_cast<const sockaddr_in*>(Unicast->Address.lpSockaddr);
      std::array<char, INET_ADDRSTRLEN> AddressBuffer = {};
      if (!inet_ntop(
              AF_INET,
              &SocketAddress->sin_addr,
              AddressBuffer.data(),
              static_cast<DWORD>(AddressBuffer.size())
          )) {
        continue;
      }

      LoggedAddress = true;
      M_LOG(
          "[NetworkUtils] IPv4 adapter candidate: adapter={} address={} prefix={} ifType={} "
          "status={} wifi={} gateway={}",
          Adapter->AdapterName ? Adapter->AdapterName : "<unknown>",
          AddressBuffer.data(),
          Unicast->OnLinkPrefixLength,
          Adapter->IfType,
          static_cast<int>(Adapter->OperStatus),
          IsWiFi,
          HasGateway
      );
      if (WiFiAddress.empty() && IsWiFi && IsUp) {
        WiFiAddress = AddressBuffer.data();
      }
    }

    if (!LoggedAddress) {
      M_LOG(
          "[NetworkUtils] Adapter has no IPv4 address: adapter={} ifType={} status={} wifi={} "
          "gateway={}",
          Adapter->AdapterName ? Adapter->AdapterName : "<unknown>",
          Adapter->IfType,
          static_cast<int>(Adapter->OperStatus),
          IsWiFi,
          HasGateway
      );
    }
  }
  return WiFiAddress;
}
std::string GetDefaultRouteIPv4() {
  SOCKET RouteSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (RouteSocket == INVALID_SOCKET) {
    M_LOG("[NetworkUtils] Default-route socket creation failed: {}", WSAGetLastError());
    return {};
  }

  sockaddr_in RouteTarget = {};
  RouteTarget.sin_family = AF_INET;
  RouteTarget.sin_port = htons(9);
  if (inet_pton(AF_INET, "8.8.8.8", &RouteTarget.sin_addr) != 1 ||
      connect(
          RouteSocket,
          reinterpret_cast<const sockaddr*>(&RouteTarget),
          static_cast<int>(sizeof(RouteTarget))
      ) == SOCKET_ERROR) {
    M_LOG("[NetworkUtils] Default-route detection failed: {}", WSAGetLastError());
    closesocket(RouteSocket);
    return {};
  }

  sockaddr_in LocalEndpoint = {};
  int EndpointSize = static_cast<int>(sizeof(LocalEndpoint));
  if (getsockname(RouteSocket, reinterpret_cast<sockaddr*>(&LocalEndpoint), &EndpointSize) ==
      SOCKET_ERROR) {
    M_LOG("[NetworkUtils] Default-route endpoint lookup failed: {}", WSAGetLastError());
    closesocket(RouteSocket);
    return {};
  }
  closesocket(RouteSocket);

  std::array<char, INET_ADDRSTRLEN> AddressBuffer = {};
  if (!inet_ntop(
          AF_INET,
          &LocalEndpoint.sin_addr,
          AddressBuffer.data(),
          static_cast<DWORD>(AddressBuffer.size())
      )) {
    M_LOG("[NetworkUtils] Default-route address conversion failed: {}", WSAGetLastError());
    return {};
  }
  return AddressBuffer.data();
}

bool IsLoopbackIPv4(const IN_ADDR& Address) {
  const unsigned char* Bytes = reinterpret_cast<const unsigned char*>(&Address.S_un.S_addr);
  return Bytes[0] == 127;
}

int GetPrivateIPv4Score(const IN_ADDR& Address) {
  const unsigned char* Bytes = reinterpret_cast<const unsigned char*>(&Address.S_un.S_addr);
  if (Bytes[0] == 192 && Bytes[1] == 168) {
    return 40;
  }
  if (Bytes[0] == 172 && Bytes[1] >= 16 && Bytes[1] <= 31) {
    return 35;
  }
  if (Bytes[0] == 10) {
    return 30;
  }
  if (!IsLoopbackIPv4(Address)) {
    return 10;
  }
  return 0;
}
}  // namespace

std::string NetworkUtils::GetLocalIPAddress() {
  WSADATA WsaData = {};
  const int StartupResult = WSAStartup(MAKEWORD(2, 2), &WsaData);
  if (StartupResult != 0) {
    M_LOG("[NetworkUtils] WSAStartup failed: {}", StartupResult);
    return {};
  }

  const std::string WiFiAddress = GetActiveWiFiIPv4();
  if (!WiFiAddress.empty()) {
    M_LOG("[NetworkUtils] Local IPv4 address selected from active Wi-Fi: {}", WiFiAddress);
    WSACleanup();
    return WiFiAddress;
  }

  M_LOG("[NetworkUtils] Active Wi-Fi address unavailable. Trying the default route.");

  const std::string DefaultRouteAddress = GetDefaultRouteIPv4();
  if (!DefaultRouteAddress.empty()) {
    M_LOG("[NetworkUtils] Local IPv4 address selected from default route: {}", DefaultRouteAddress);
    WSACleanup();
    return DefaultRouteAddress;
  }

  M_LOG("[NetworkUtils] Default-route address unavailable. Falling back to host addresses.");
  char HostName[256] = {};
  if (gethostname(HostName, static_cast<int>(sizeof(HostName))) != 0) {
    M_LOG("[NetworkUtils] gethostname failed: {}", WSAGetLastError());
    WSACleanup();
    return {};
  }

  addrinfo Hints = {};
  Hints.ai_family = AF_INET;
  Hints.ai_socktype = SOCK_DGRAM;

  addrinfo* ResultList = nullptr;
  const int AddrInfoResult = getaddrinfo(HostName, nullptr, &Hints, &ResultList);
  if (AddrInfoResult != 0) {
    M_LOG("[NetworkUtils] getaddrinfo failed: {}", AddrInfoResult);
    WSACleanup();
    return {};
  }

  std::string BestAddress;
  int BestScore = -1;
  for (addrinfo* It = ResultList; It; It = It->ai_next) {
    if (!It->ai_addr || It->ai_family != AF_INET) {
      continue;
    }

    const auto* SocketAddress = reinterpret_cast<const sockaddr_in*>(It->ai_addr);
    const IN_ADDR Address = SocketAddress->sin_addr;
    const int Score = GetPrivateIPv4Score(Address);
    if (Score <= BestScore) {
      continue;
    }

    std::array<char, INET_ADDRSTRLEN> AddressBuffer = {};
    if (!inet_ntop(
            AF_INET, &Address, AddressBuffer.data(), static_cast<DWORD>(AddressBuffer.size())
        )) {
      continue;
    }

    BestScore = Score;
    BestAddress = AddressBuffer.data();
  }

  freeaddrinfo(ResultList);
  WSACleanup();

  if (BestAddress.empty()) {
    M_LOG("[NetworkUtils] Local IPv4 address was not found.");
  } else {
    M_LOG("[NetworkUtils] Fallback local IPv4 address selected: {}", BestAddress);
  }

  return BestAddress;
}