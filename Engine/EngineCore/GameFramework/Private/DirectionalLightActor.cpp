#include "DirectionalLightActor.h"

#include "DirectionalLightComponent.h"

REGISTER_ACTOR(ADirectionalLightActor);

ADirectionalLightActor::ADirectionalLightActor() {
  LightComponent = NewObject<MDirectionalLightComponent>(this);
  SetRootComponent(LightComponent);
  if (LightComponent) LightComponent->RegisterComponent();
}

ADirectionalLightActor::~ADirectionalLightActor() = default;

void ADirectionalLightActor::OnColorChanged(FColor OldValue) {
  (void)OldValue;
  if (LightComponent) LightComponent->SetColor(Color);
}

void ADirectionalLightActor::OnIntensityChanged(float OldValue) {
  (void)OldValue;
  if (LightComponent) LightComponent->SetIntensity(Intensity);
}

void ADirectionalLightActor::OnEnabledChanged(bool OldValue) {
  (void)OldValue;
  if (LightComponent) LightComponent->SetEnabled(Enabled);
}
