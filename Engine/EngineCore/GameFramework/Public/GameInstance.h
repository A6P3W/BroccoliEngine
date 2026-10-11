#pragma once
#include "BaseObject.h"
#include "BroccoliEngineAPI.h"
#include "NetworkTypes.h"

class BROCCOLI_ENGINE_API GameInstance : public MBaseObject {
 public:
  virtual ~GameInstance() = default;
  virtual void OnSessionDisconnected(ESessionDisconnectReason Reason);
};
