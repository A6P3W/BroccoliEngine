#include "Components/Component.h"

Component::~Component() = default;

void Component::Update(float DeltaTime)
{
}

void Component::Draw()
{
}

void Component::OnMessage(const std::string& message)
{
}

void Component::SetOwner(OUpdateableObject* owner)
{
    m_owner = owner;
}
