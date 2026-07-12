#pragma once
#include "BroccoliEngineAPI.h"
 
 #include <cstdint>
 
 #include "MovementComponent.h"
 #include "NetworkTypes.h"
 #include "UMath.h"
 
 class BROCCOLI_ENGINE_API MNetMovementComponent : public MMovementComponent {
  public:
   MNetMovementComponent();
   ~MNetMovementComponent() override;
 
   void OnRegister() override;
   void OnUpdate(float DeltaTime) override;
 
   void AddMovementInput(const FVector2D& Input);
   void AddWorldForce(const FVector2D& Force) override;
   void AddLocalForce(const FVector2D& Force) override;
   void SetWorldForce(const FVector2D& Force) override;
   void SetLocalForce(const FVector2D& Force) override;
   void AddVelocityRotation(const FRotator& Rotation) override;
   void SetVelocityRotation(const FRotator& Rotation) override;
 
   void SetMaxSpeed(float NewMaxSpeed);
   float GetMaxSpeed() const;
   void SetAcceleration(float NewAcceleration);
   float GetAcceleration() const;
 
  protected:
   virtual void SimulateMovement(const FMovePredictionData& Move);
   void ApplyMovementData(const FVector2D& DesiredOffset);
 
   void Server_UploadMove(FMovePredictionData Move);
   void Client_ResolveMove(
       uint32_t AckedSequence,
       FVector2D ServerLocation,
       FRotator ServerRotation,
       FVector2D ServerVelocity
   );
   bool ShouldSimulate() const;
   void ClearFrameMovementData();
 
  private:
   struct Impl;
   Impl* ImplPtr = nullptr;
 };
