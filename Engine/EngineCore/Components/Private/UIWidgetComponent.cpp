#include "UIWidgetComponent.h"

MUIWidgetComponent::MUIWidgetComponent(int basePriority)
	: m_basePriority(basePriority)
{}

void MUIWidgetComponent::SetZOrderOffset(int offset)
{
	m_zOrderOffset = offset;

	for (auto* child : m_childComponents) {
		if (auto* childWidget = dynamic_cast<MUIWidgetComponent*>(child)) {
			childWidget->SetZOrderOffset(offset);
		}
	}
}

int MUIWidgetComponent::GetFinalPriority() const
{
	return m_basePriority + m_zOrderOffset;
}
