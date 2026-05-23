#include "RenderSystem.h"
#include "DxLib.h"
#include <algorithm>
#include <cmath>
#include "CameraComponent.h"
#include "Utils/UMath.h"

RenderSystem::RenderSystem()
{

}
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

void RenderSystem::SubmitBox(float x1, float y1, float x2, float y2, int color, int fill, RenderSpace space, int priority, int alpha)
{
	RenderCommand command;
	command.type = RenderType::Box;
	command.x1 = x1;
	command.y1 = y1;
	command.x2 = x2;
	command.y2 = y2;
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
	command.x1 = destX; // 描画先のX座標
	command.y1 = destY; // 描画先のY座標
	command.srcX = srcX; // 元画像の切り出し開始X座標
	command.srcY = srcY; // 元画像の切り出し開始Y座標
	command.srcWidth = srcW; // 元画像の切り出し幅
	command.srcHeight = srcH; // 元画像の切り出し高さ
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

	int windowWidth = 0;
	int windowHeight = 0;
	GetDrawScreenSize(&windowWidth, &windowHeight);
	const float centerX = windowWidth * 0.5f;
	const float centerY = windowHeight * 0.5f;

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

	int windowWidth = 0;
	int windowHeight = 0;
	GetDrawScreenSize(&windowWidth, &windowHeight);
	const float centerX = windowWidth * 0.5f;
	const float centerY = windowHeight * 0.5f;

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
			float renderX = command.x1;
			float renderY = command.y1;
			float optX = command.x2;
			float optY = command.y2;
			float finalScale = command.Scale;
			float finalRadius = command.x2;
			double drawAngle = command.AngleDeg;

			if (command.alpha != current_alpha) {
				int normalizedAlpha = std::clamp(command.alpha, 0, 255);
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, normalizedAlpha);
				current_alpha = normalizedAlpha;
			}

			switch (command.space) {
			case RenderSpace::World: {
				const FVector2D screenPos = WorldToScreen({ renderX, renderY });
				renderX = screenPos.X;
				renderY = screenPos.Y;

				if (command.type == RenderType::Box || command.type == RenderType::Line) {
					const FVector2D optScreenPos = WorldToScreen({ optX, optY });
					optX = optScreenPos.X;
					optY = optScreenPos.Y;
				}

				finalScale *= camFOV;
				finalRadius *= camFOV;
				drawAngle -= UMath::DegToRad(camAngle); // カメラの回転を反映
				break;
			}
			case RenderSpace::Screen:
				// スクリーン空間の描画はそのまま
				break;
			}

			switch (command.type) {
			case RenderType::Graph:
				DrawRotaGraphFastF(static_cast<float>(renderX), static_cast<float>(renderY), finalScale, drawAngle, command.handle, TRUE);
				break;
			case RenderType::Box:
				if (command.space == RenderSpace::World) {
					const FVector2D v1 = WorldToScreen({ command.x1, command.y1 });
					const FVector2D v2 = WorldToScreen({ command.x2, command.y1 });
					const FVector2D v3 = WorldToScreen({ command.x2, command.y2 });
					const FVector2D v4 = WorldToScreen({ command.x1, command.y2 });

					DrawQuadrangle(
						static_cast<int>(v1.X), static_cast<int>(v1.Y),
						static_cast<int>(v2.X), static_cast<int>(v2.Y),
						static_cast<int>(v3.X), static_cast<int>(v3.Y),
						static_cast<int>(v4.X), static_cast<int>(v4.Y),
						command.color, command.fill);
				} else {
					DrawBox(static_cast<int>(renderX), static_cast<int>(renderY), static_cast<int>(optX), static_cast<int>(optY), command.color, command.fill);
				}
				break;
			case RenderType::Text:
				DrawStringToHandle(static_cast<int>(renderX), static_cast<int>(renderY), command.text.c_str(), command.color, command.handle);
				break;
			case RenderType::Line:
				DrawLine(static_cast<int>(renderX), static_cast<int>(renderY), static_cast<int>(optX), static_cast<int>(optY), command.color);
				break;
			case RenderType::Circle:
				DrawCircle(static_cast<int>(renderX), static_cast<int>(renderY), static_cast<int>(finalRadius), command.color, command.fill);
				break;
			case RenderType::RectGraph:
				DrawRectGraph(static_cast<int>(renderX), static_cast<int>(renderY),
					static_cast<int>(command.srcX), static_cast<int>(command.srcY),
					static_cast<int>(command.srcWidth), static_cast<int>(command.srcHeight),
					command.handle, TRUE);
				break;
			}
		}
	}
	m_priorityCommands.clear(); // 全コマンドをクリア
}

void RenderSystem::SetCameraView(MCameraComponent* m)
{
	m_MainCamera = m;
}

MCameraComponent* RenderSystem::GetCamera()
{
	return m_MainCamera;
}
