#include "SpriteComponent.h"
#include "Actor.h"
#include "Utils/UMath.h"
#include "ResourceManager.h"

MSpriteComponent::MSpriteComponent(int priority, RenderSpace space)
{
	m_command.priority = priority;
	m_command.space = space;

}

void MSpriteComponent::SubmitGraph(double Scale, int handle, int alpha)
{
	m_command.type = RenderType::Graph;
	m_command.Scale = Scale;
	m_command.handle = handle;
	m_command.alpha = alpha;
}

void MSpriteComponent::SubmitBox(float width, float height, int color, int fill, int alpha)
{
	m_command.type = RenderType::Box;
	m_command.x2 = width;
	m_command.y2 = height;
	m_command.color = color;
	m_command.fill = fill;
	m_command.alpha = alpha;
}

void MSpriteComponent::SubmitText(const std::string& text, int color, int handle, int alpha)
{
	m_command.type = RenderType::Text;
	m_command.text = text;
	m_command.color = color;
	if (handle != -1) {
		m_command.handle = handle;
	} else {
		m_command.handle = ResourceManager::GetInstance().GetFont(12, 5); 
	}
	m_command.alpha = alpha;
}

void MSpriteComponent::SubmitLine(float x2, float y2, int color, int alpha)
{
	m_command.type = RenderType::Line;
	m_command.x2 = x2;
	m_command.y2 = y2;
	m_command.color = color;
	m_command.alpha = alpha;
}

void MSpriteComponent::SubmitRectGraph(float srcX, float srcY, float srcW, float srcH, int handle, int alpha)
{
	m_command.type = RenderType::RectGraph;
	m_command.srcX = srcX;
	m_command.srcY = srcY;
	m_command.srcWidth = srcW;
	m_command.srcHeight = srcH;
	m_command.handle = handle;
	m_command.alpha = alpha;
}

void MSpriteComponent::SubmitCircle(float radius, int color, int fill, int alpha)
{
	m_command.type = RenderType::Circle;
	m_command.x2 = radius;
	m_command.color = color;
	m_command.fill = fill;
	m_command.alpha = alpha;
}

void MSpriteComponent::Draw()
{
    const FVector2D& WorldLocation = GetWorldLocation();
	const FRotator& WorldRotation = GetWorldRotation();
	double WorldRadRotation = UMath::DegToRad(WorldRotation.Rotation);
	RenderCommand command = m_command;

	command.x1 = WorldLocation.X;
	command.y1 = WorldLocation.Y;

	if (command.type == RenderType::Box) {
		command.x2 = WorldLocation.X + command.x2;
		command.y2 = WorldLocation.Y + command.y2;
	} else if (command.type == RenderType::Line) {
		command.x2 = WorldLocation.X + command.x2;
		command.y2 = WorldLocation.Y + command.y2;
	}

	if (command.type == RenderType::Graph) {
		command.Scale *= GetScale();
		command.AngleDeg = WorldRadRotation;
	}

	switch (command.type) {
	case RenderType::Graph:
		RenderSystem::GetInstance().SubmitGraph(
			command.x1,
			command.y1,
			command.Scale,
			command.AngleDeg,
			command.handle,
			command.space,
			command.priority,
			command.alpha);
		break;
	case RenderType::Box:
		RenderSystem::GetInstance().SubmitBox(
			command.x1,
			command.y1,
			command.x2,
			command.y2,
			command.color,
			command.fill,
			command.space,
			command.priority,
			command.alpha);
		break;
	case RenderType::Text:
		RenderSystem::GetInstance().SubmitText(
			command.text,
			command.x1,
			command.y1,
			command.color,
			command.handle,
			command.space,
			command.priority,
			command.alpha);
		break;
	case RenderType::Line:
		RenderSystem::GetInstance().SubmitLine(
			command.x1,
			command.y1,
			command.x2,
			command.y2,
			command.color,
			command.space,
			command.priority,
			command.alpha);
		break;
	case RenderType::RectGraph:
		RenderSystem::GetInstance().SubmitRectGraph(
			command.x1,
			command.y1,
			command.srcX,
			command.srcY,
			command.srcWidth,
			command.srcHeight,
			command.handle,
			command.space,
			command.priority,
			command.alpha);
		break;
	case RenderType::Circle:
		RenderSystem::GetInstance().SubmitCircle(
			command.x1,
			command.y1,
			command.x2,
			command.color,
			command.fill,
			command.space,
			command.priority,
			command.alpha);
		break;
	}
}


void MSpriteComponent::OnMessage(const std::string& message)
{
	if (message == "HIT") {
		// 当たった時に何か演出をするなどの処理
	}
}
