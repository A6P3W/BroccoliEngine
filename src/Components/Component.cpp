#include "Components/Component.h"

Component::~Component() = default;

void Component::Update()
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
