#pragma once
#include "BaseObject.h"
#include "EOSTypes.h"

class GameInstance : public MBaseObject {
 public:
  virtual ~GameInstance() = default;
  virtual void OnSessionDisconnected(ELobbyDisconnectReason Reason);
};
