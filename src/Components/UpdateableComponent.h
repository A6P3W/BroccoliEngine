#pragma once
#include "BaseComponent.h"
class UpdateableComponent :
    public MBaseComponent
{
public:
    virtual bool Update(float DeltaTime) final;

private:

protected:
    virtual void OnUpdate(float DeltaTime) {}
};

