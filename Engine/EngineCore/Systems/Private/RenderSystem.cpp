#include "RenderSystem.h"
#include "DxLib.h"
#include <algorithm>
#include <cmath>
#include "CameraComponent.h"
#include "Utils/UMath.h"
#include "EngineDefine.h"

RenderSystem::RenderSystem()
{}

RenderSystem& RenderSystem::GetInstance()
{
	static RenderSystem instance;
	return instance;
}

void RenderSystem::SubmitGraph(float x, float y, double Scale, double AngleDeg, int handle, RenderSpace space, int priority, int alpha)
{
	RenderCommand command;
	command.type = RenderType::Graph;
	command.x1 = x;
	command.y1 = y;
	command.Scale = Scale;
	command.AngleDeg = AngleDeg;
	command.handle = handle;
	command.space = space;
	command.priority = priority;
	command.alpha = alpha;
	m_priorityCommands[priority].push_back(std::move(command));
}

void RenderSystem::SubmitCircle(float x, float y, float radius, int color, int fill, RenderSpace space, int priority, int alpha)
{
	RenderCommand command;
	command.type = RenderType::Circle;
	command.x1 = x;
	command.y1 = y;
	command.x2 = radius;
	command.color = color;
	command.fill = fill;
	command.space = space;
	command.priority = priority;
	command.alpha = alpha;
	m_priorityCommands[priority].push_back(std::move(command));
}

void RenderSystem::SubmitBox(float x1, float y1, float x2, float y2, float AngleDeg, int color, int fill, RenderSpace space, int priority, int alpha)
{
	RenderCommand command;
	command.type = RenderType::Box;
	command.x1 = x1;
	command.y1 = y1;
	command.x2 = x2;
	command.y2 = y2;
	command.AngleDeg = AngleDeg;
	command.color = color;
	command.fill = fill;
	command.space = space;
	command.priority = priority;
	command.alpha = alpha;
	m_priorityCommands[priority].push_back(std::move(command));
}

void RenderSystem::SubmitText(const std::string& text, float x, float y, int color, int handle, RenderSpace space, int priority, int alpha)
{
	RenderCommand command;
	command.type = RenderType::Text;
	command.text = text;
	command.x1 = x;
	command.y1 = y;
	command.handle = handle;
	command.color = color;
	command.space = space;
	command.priority = priority;
	command.alpha = alpha;
	m_priorityCommands[priority].push_back(std::move(command));
}

void RenderSystem::SubmitLine(float x1, float y1, float x2, float y2, int color, RenderSpace space, int priority, int alpha)
{
	RenderCommand command;
	command.type = RenderType::Line;
	command.x1 = x1; command.y1 = y1;
	command.x2 = x2; command.y2 = y2;
	command.color = color;
	command.space = space;
	command.priority = priority;
	command.alpha = alpha;
	m_priorityCommands[priority].push_back(std::move(command));
}

void RenderSystem::SubmitRectGraph(float destX, float destY, float srcX, float srcY, float srcW, float srcH, int handle, RenderSpace space, int priority, int alpha)
{
	RenderCommand command;
	command.type = RenderType::RectGraph;
	command.x1 = destX;
	command.y1 = destY;
	command.srcX = srcX;
	command.srcY = srcY;
	command.srcWidth = srcW;
	command.srcHeight = srcH;
	command.handle = handle;
	command.space = space;
	command.priority = priority;
	command.alpha = alpha;
	m_priorityCommands[priority].push_back(std::move(command));
}

FVector2D RenderSystem::WorldToScreen(const FVector2D& worldPosition) const
{
	FVector2D camPos = FVector2D::ZeroVector;
	float camAngle = 0.0f;
	float camFOV = 1.0f;
	if (m_MainCamera) {
		camPos = m_MainCamera->GetWorldLocation();
		camAngle = m_MainCamera->GetWorldRotation().Rotation;
		camFOV = m_MainCamera->GetFOV();
	}

	// 仮想解像度を基準にする
	const float centerX = VirtualWidth * 0.5f;
	const float centerY = VirtualHeight * 0.5f;

	const float localX = worldPosition.X - camPos.X;
	const float localY = worldPosition.Y - camPos.Y;

	const float rad = UMath::DegToRad(-camAngle);
	const float cosTheta = std::cos(rad);
	const float sinTheta = std::sin(rad);

	const float rotatedX = localX * cosTheta - localY * sinTheta;
	const float rotatedY = localX * sinTheta + localY * cosTheta;

	return {
		rotatedX * camFOV + centerX,
		rotatedY * camFOV + centerY
	};
}

FVector2D RenderSystem::ScreenToWorld(const FVector2D& screenPosition) const
{
	FVector2D camPos = FVector2D::ZeroVector;
	float camAngle = 0.0f;
	float camFOV = 1.0f;
	if (m_MainCamera) {
		camPos = m_MainCamera->GetWorldLocation();
		camAngle = m_MainCamera->GetWorldRotation().Rotation;
		camFOV = m_MainCamera->GetFOV();
	}

	// 仮想解像度を基準にする
	const float centerX = VirtualWidth * 0.5f;
	const float centerY = VirtualHeight * 0.5f;

	const float safeFOV = (std::abs(camFOV) < 1.0e-6f) ? 1.0e-6f : camFOV;
	const float localX = (screenPosition.X - centerX) / safeFOV;
	const float localY = (screenPosition.Y - centerY) / safeFOV;

	const float rad = UMath::DegToRad(camAngle);
	const float cosTheta = std::cos(rad);
	const float sinTheta = std::sin(rad);

	const float rotatedX = localX * cosTheta - localY * sinTheta;
	const float rotatedY = localX * sinTheta + localY * cosTheta;

	return {
		rotatedX + camPos.X,
		rotatedY + camPos.Y
	};
}


void RenderSystem::Draw()
{
	float camAngle = 0.0f;
	float camFOV = 1.0f;
	if (m_MainCamera) {
		camAngle = m_MainCamera->GetWorldRotation().Rotation;
		camFOV = m_MainCamera->GetFOV();
	}

	int current_blendmode, current_alpha;
	GetDrawBlendMode(&current_blendmode, &current_alpha);

	for (const auto& pair : m_priorityCommands) {
		for (const auto& command : pair.second) {
			// アルファ値の設定
			if (command.alpha != current_alpha) {
				int normalizedAlpha = std::clamp(command.alpha, 0, 255);
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, normalizedAlpha);
				current_alpha = normalizedAlpha;
			}

			if (command.space == RenderSpace::World && command.type == RenderType::Box) {
				float worldW = command.x2 - command.x1;
				float worldH = command.y2 - command.y1;

				double objAngleRad = command.AngleDeg;
				float cosA = static_cast<float>(std::cos(objAngleRad));
				float sinA = static_cast<float>(std::sin(objAngleRad));

				auto GetProjectedVertex = [&](float lx, float ly) -> FVector2D {
					float rx = lx * cosA - ly * sinA;
					float ry = lx * sinA + ly * cosA;

					return WorldToScreen({ command.x1 + rx, command.y1 + ry });
					};

				FVector2D v1 = GetProjectedVertex(0, 0);           // 左上
				FVector2D v2 = GetProjectedVertex(worldW, 0);      // 右上
				FVector2D v3 = GetProjectedVertex(worldW, worldH); // 右下
				FVector2D v4 = GetProjectedVertex(0, worldH);      // 左下

				DrawQuadrangle(
					static_cast<int>(v1.X), static_cast<int>(v1.Y),
					static_cast<int>(v2.X), static_cast<int>(v2.Y),
					static_cast<int>(v3.X), static_cast<int>(v3.Y),
					static_cast<int>(v4.X), static_cast<int>(v4.Y),
					command.color, command.fill);

				continue;
			}

			float renderX = command.x1;
			float renderY = command.y1;
			float optX = command.x2;
			float optY = command.y2;
			float finalScale = command.Scale;
			float finalRadius = command.x2;
			double drawAngle = command.AngleDeg;

			if (command.space == RenderSpace::World) {
				const FVector2D screenPos = WorldToScreen({ renderX, renderY });
				renderX = screenPos.X;
				renderY = screenPos.Y;
				if (command.type == RenderType::Line) {
					const FVector2D optScreenPos = WorldToScreen({ optX, optY });
					optX = optScreenPos.X;
					optY = optScreenPos.Y;
				}
				finalScale *= camFOV;
				finalRadius *= camFOV;
				drawAngle -= UMath::DegToRad(camAngle);
			}

			switch (command.type) {
			case RenderType::Graph:
				DrawRotaGraphFastF(renderX, renderY, (float)finalScale, (float)drawAngle, command.handle, TRUE);
				break;
			case RenderType::Text:
				DrawStringToHandle((int)renderX, (int)renderY, command.text.c_str(), command.color, command.handle);
				break;
			case RenderType::Line:
				DrawLine((int)renderX, (int)renderY, (int)optX, (int)optY, command.color);
				break;
			case RenderType::Circle:
				DrawCircle((int)renderX, (int)renderY, (int)finalRadius, command.color, command.fill);
				break;
			case RenderType::Box:
				DrawBox((int)renderX, (int)renderY, (int)optX, (int)optY, command.color, command.fill);
				break;
			}
		}
	}
	m_priorityCommands.clear();
}

void RenderSystem::SetCameraView(MCameraComponent* m)
{
	m_MainCamera = m;
}

MCameraComponent* RenderSystem::GetCamera()
{
	return m_MainCamera;
}
