#pragma once

#include "ActorComponent.h"
#include "BroccoliEngineAPI.h"
#include "UMath.h"

enum class ERigidBody3DType : uint8_t { Static, Kinematic, Dynamic };

class BROCCOLI_ENGINE_API MRigidBody3DComponent : public MActorComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MRigidBody3DComponent)

  ERigidBody3DType GetBodyType() const { return BodyType; }
  void SetBodyType(ERigidBody3DType NewBodyType);
  float GetMass() const { return Mass; }
  void SetMass(float NewMass);
  FVector3D GetLinearVelocity() const;
  void SetLinearVelocity(const FVector3D& NewLinearVelocity);
  void AddForce(const FVector3D& Force);
  void AddImpulse(const FVector3D& Impulse);
  bool IsRegisteredWithPhysics() const { return bRegisteredWithPhysics; }

 protected:
  void OnRegister() override;
  void OnUnregister() override;

 private:
  friend class FPhysicsSystem3D;
  void SetRegisteredWithPhysics(bool NewValue) { bRegisteredWithPhysics = NewValue; }
  void SyncTransformFromPhysics(const FVector3D& Location, const FQuaternion& Rotation);

  ERigidBody3DType BodyType = ERigidBody3DType::Static;
  float Mass = 1.0f;
  bool bRegisteredWithPhysics = false;
};
