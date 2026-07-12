#pragma once
#include "BaseObject.h"
#include "EOSTypes.h"
#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API GameInstance : public MBaseObject {
 public:
  virtual ~GameInstance() = default;
  virtual void OnSessionDisconnected(ELobbyDisconnectReason Reason);
};
