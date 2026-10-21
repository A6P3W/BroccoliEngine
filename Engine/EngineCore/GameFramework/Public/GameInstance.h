#pragma once
#include "BaseObject.h"
#include "BroccoliEngineAPI.h"
#include "NetworkTypes.h"

/// @brief レベル移動をまたいで保持されるクラス。
class BROCCOLI_ENGINE_API GameInstance : public MBaseObject {
 public:
  virtual ~GameInstance() = default;
  virtual void OnSessionDisconnected(ESessionDisconnectReason Reason);
};
