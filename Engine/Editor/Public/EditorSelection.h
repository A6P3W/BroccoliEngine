#pragma once

class AActor;
class EditorSelectPointComponent;
class World;
struct FVector2D;

class EditorSelection {
 public:
  AActor* HitTest2D(World* WorldPtr, const FVector2D& WorldPosition) const;
  void Select(AActor* Actor);
  void Clear();

  AActor* GetSelectedActor() const { return SelectedActor; }

 private:
  AActor* SelectedActor = nullptr;
  EditorSelectPointComponent* SelectedPointComponent = nullptr;
};
