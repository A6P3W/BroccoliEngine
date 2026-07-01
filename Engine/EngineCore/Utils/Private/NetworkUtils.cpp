#include "NetworkUtils.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <WinSock2.h>
#include <Ws2tcpip.h>

#include <array>

#include "Log.h"

namespace {
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
    if (!inet_ntop(AF_INET, &Address, AddressBuffer.data(), static_cast<DWORD>(AddressBuffer.size()))) {
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
    M_LOG("[NetworkUtils] Local IPv4 address selected: {}", BestAddress);
  }

  return BestAddress;
}