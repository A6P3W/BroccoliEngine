#pragma once

#include <string>

#include "UMath.h"

class AActor;
class World;

class EditorPlacementTool {
 public:
  AActor* Place2D(World* WorldPtr, const std::string& ClassName, const FVector2D& Position) const;
  AActor* Place3D(World* WorldPtr, const std::string& ClassName, const FVector3D& Position) const;
};
