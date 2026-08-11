#include "TintTestActor.h"

#include <algorithm>
#include <cmath>

#include "ResourceManager.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(ATintTestActor)

ATintTestActor::ATintTestActor() {
  SpriteComponent = NewObject<MSpriteComponent>(this);
  SetRootComponent(SpriteComponent);
  if (SpriteComponent == nullptr) {
    return;
  }

  const int TextureHandle =
      ResourceManager::GetInstance().LoadResourceGraph("/Engine/texture_Checker_64px.png");
  SpriteComponent->SubmitGraph(TextureHandle);
  SpriteComponent->RegisterComponent();
}

void ATintTestActor::OnUpdate(float DeltaTime) {
  if (SpriteComponent == nullptr) {
    return;
  }

  ElapsedTime += DeltaTime;
  const auto ToColorChannel = [](float Phase) {
    return static_cast<uint8_t>(std::clamp((std::sin(Phase) * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
  };

  SpriteComponent->SetTint(
      FColor{
          ToColorChannel(ElapsedTime),
          ToColorChannel(ElapsedTime + 2.0943951f),
          ToColorChannel(ElapsedTime + 4.1887902f)
      }
  );
}
