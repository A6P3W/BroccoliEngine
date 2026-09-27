#pragma once

#include "Actor.h"
#include "ReflectionGenerator.h"

class MSpriteComponent;

class ATintTestActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ATintTestActor)

  ATintTestActor();

 protected:
  virtual void OnSpeedChanged(float OldValue);

 public:
  EDITOR_PROPERTY(
      FFloatEditorProperty{
          .Base = {.OnChanged = ^^ATintTestActor::OnSpeedChanged},
          .Min = 0.0f,
          .Max = 1000.0f,
          .SliderMin = 0.0f,
          .SliderMax = 100.0f
      }
  )
  float Speed = 1.0f;
  EDITOR_PROPERTY(FIntEditorProperty{.Min = 0, .Max = 100}) int Level = 0;
  EDITOR_PROPERTY(FBoolEditorProperty{}) bool Enabled = true;
  EDITOR_PROPERTY(FStringEditorProperty{.MaxLength = 32}) std::string DisplayName = "Tint test";
  EDITOR_PROPERTY(FVector2DEditorProperty{}) FVector2D Offset2D {};
  EDITOR_PROPERTY(FVector3DEditorProperty{}) FVector3D Target3D {};

 protected:
  void OnUpdate(float DeltaTime) override;

 private:
  MSpriteComponent* SpriteComponent = nullptr;
  float ElapsedTime = 0.0f;
};
