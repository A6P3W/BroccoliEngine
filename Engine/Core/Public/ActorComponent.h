#pragma once
#include "BaseObject.h"
#include <string>
#include <vector>
#include <memory>

class MActorComponent :
	public MBaseObject
{
public:
	
	bool Update(float DeltaTime) ;
	virtual void Draw() {}
private:
	
protected:
	virtual void OnUpdate(float DeltaTime) {}
};

