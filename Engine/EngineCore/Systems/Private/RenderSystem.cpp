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

void RenderSystem::Draw()
{

	FVector2D CamPos = FVector2D::ZeroVector;
	float camAngle = 0.0f;
	float camFOV = 1.0f;
	MATRIX viewMat = MGetIdent();
	if (m_MainCamera) {
		CamPos = m_MainCamera->GetWorldLocation();
		camAngle = m_MainCamera->GetWorldRotation().Rotation;
		camFOV = m_MainCamera->GetFOV();
		MATRIX scaleMat = MGetScale(VGet(camFOV, camFOV, 1.0f));
		MATRIX matTrans = MGetTranslate(VGet(-CamPos.X, -CamPos.Y, 0.0f));
		MATRIX matRot = MGetRotZ(UMath::DegToRad(-camAngle));
		viewMat = MMult(matTrans, matRot);
		viewMat = MMult(viewMat, scaleMat);
	}
	int WindowWidth, WindowHeight;
	GetDrawScreenSize(&WindowWidth, &WindowHeight);
	const float centerX = WindowWidth * 0.5f;
	const float centerY = WindowHeight * 0.5f;
	MATRIX screenMat = MGetTranslate(VGet(centerX, centerY, 0.0f));
	MATRIX renderMat = MMult(viewMat, screenMat);
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
				VECTOR v1 = VGet(renderX, renderY, 0.0f);

				v1 = VTransform(v1, renderMat);
				renderX = v1.x; renderY = v1.y;

				if (command.type == RenderType::Box || command.type == RenderType::Line) {
					VECTOR v2_transformed = VTransform(VGet(optX, optY, 0.0f), renderMat);
					optX = v2_transformed.x;
					optY = v2_transformed.y;
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
					VECTOR v1 = VTransform(VGet(command.x1, command.y1, 0.0f), renderMat);
					VECTOR v2 = VTransform(VGet(command.x2, command.y1, 0.0f), renderMat);
					VECTOR v3 = VTransform(VGet(command.x2, command.y2, 0.0f), renderMat);
					VECTOR v4 = VTransform(VGet(command.x1, command.y2, 0.0f), renderMat);

					DrawQuadrangle(
						static_cast<int>(v1.x), static_cast<int>(v1.y),
						static_cast<int>(v2.x), static_cast<int>(v2.y),
						static_cast<int>(v3.x), static_cast<int>(v3.y),
						static_cast<int>(v4.x), static_cast<int>(v4.y),
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
