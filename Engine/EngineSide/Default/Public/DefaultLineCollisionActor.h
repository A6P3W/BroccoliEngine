#pragma once

#include "Actor.h"

// Default: Line collision component only.
class ADefaultLineCollisionActor : public AActor
{
public:
	DEFINE_ACTOR_CLASS(ADefaultLineCollisionActor)
	ADefaultLineCollisionActor();
};
