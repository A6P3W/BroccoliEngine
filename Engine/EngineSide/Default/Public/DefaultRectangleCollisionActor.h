#pragma once

#include "Actor.h"

// Default: Rectangle collision component only.
class ADefaultRectangleCollisionActor : public AActor
{
public:
	DEFINE_ACTOR_CLASS(ADefaultRectangleCollisionActor)
	ADefaultRectangleCollisionActor();
};
