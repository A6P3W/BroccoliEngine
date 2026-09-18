#pragma once

#include <string>

#include "Actor.h"

class MStaticMeshComponent;

class BROCCOLI_ENGINE_API AStaticMeshActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AStaticMeshActor);

  AStaticMeshActor();
  ~AStaticMeshActor() override;

  void SetModelPath(const std::string& Path);
  const std::string& GetModelPath() const;

 protected:
  void BeginPlay() override;

 private:
  MStaticMeshComponent* StaticMeshComponent = nullptr;
  std::string ModelPath;
};
