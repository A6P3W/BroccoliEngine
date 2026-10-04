#include "ComponentRegistry.h"

class MAbstractFactoryComponent : public MActorComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MAbstractFactoryComponent)
  virtual void Required() = 0;
};

bool InvalidRegistration =
    ComponentRegistry::GetInstance().RegisterOwned<MAbstractFactoryComponent>("Test");
