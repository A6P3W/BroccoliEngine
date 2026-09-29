#pragma once

#include "Actor.h"
#include "ReflectionGenerator.h"

class MSprite2DComponent;

class ATintTestActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ATintTestActor)

  ATintTestActor();

 protected:
  virtual void OnSpeedChanged(float OldValue);

 private:
  EDITOR_PROPERTY(
          .OnEditorChanged = ^^ATintTestActor::OnSpeedChanged,
          .Min = 0.0f,
          .Max = 1000.0f,
          .SliderMin = 0.0f,
          .SliderMax = 100.0f
  )
  float Speed = 1.0f;
  EDITOR_PROPERTY(.Min = 0, .Max = 100)
  int Level = 0;
  EDITOR_PROPERTY()
  bool Enabled = true;
  EDITOR_PROPERTY(.MaxLength = 32)
  std::string DisplayName = "Tint test";
  EDITOR_PROPERTY()
  FVector2D Offset2D{};
  EDITOR_PROPERTY()
  FVector3D Target3D{};

 protected:
  void OnUpdate(float DeltaTime) override;

 private:
  MSprite2DComponent* SpriteComponent = nullptr;
  float ElapsedTime = 0.0f;
};
