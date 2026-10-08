#include "ActorComponent.h"
#include "ComponentRegistry.h"

class MNonDefaultFactoryComponent : public MActorComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MNonDefaultFactoryComponent)
  explicit MNonDefaultFactoryComponent(int) {}
};

bool InvalidRegistration =
    ComponentRegistry::GetInstance().RegisterOwned<MNonDefaultFactoryComponent>("Test");
