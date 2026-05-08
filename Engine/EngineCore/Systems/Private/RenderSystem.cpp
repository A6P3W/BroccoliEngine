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
	m_commands.push_back(std::move(command));
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
	m_commands.push_back(std::move(command));
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
	m_commands.push_back(std::move(command));
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
	m_commands.push_back(std::move(command));
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
	m_commands.push_back(std::move(command));
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
	m_commands.push_back(std::move(command));
}

void RenderSystem::Draw()
{
	std::stable_sort(m_commands.begin(), m_commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
		return a.priority < b.priority;
		});
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

	const float camAngleRad = UMath::DegToRad(camAngle);

	MATRIX screenMat = MGetTranslate(VGet(centerX, centerY, 0.0f));

	MATRIX renderMat = MMult(viewMat, screenMat);

	for (const auto& command : m_commands) {
		float renderX = command.x1;
		float renderY = command.y1;
		float optX = command.x2;
		float optY = command.y2;
		float finalScale = command.Scale ;
		float finalRadius = command.x2;
		double drawAngle = command.AngleDeg;
		int normalizedAlpha = std::clamp(command.alpha, 0, 255);

		float rx1 = 0.0f, ry1 = 0.0f;

		switch (command.space) {
		case RenderSpace::World: {
			VECTOR v1 = VGet(renderX, renderY, 0.0f);
			VECTOR v2 = VGet(optX, optY, 0.0f);
			optX = v2.x; optY = v2.y;
			v1 = VTransform(v1, renderMat);
			renderX = v1.x; renderY = v1.y;
			if (command.type == RenderType::Box || command.type == RenderType::Line) {
				VECTOR v2 = VTransform(VGet(optX, optY, 0.0f), renderMat);
				optX = v2.x;
				optY = v2.y;
			}

			finalScale *= camFOV;
			finalRadius *= camFOV;

			drawAngle -= camAngleRad;
			break;
		}
		case RenderSpace::Screen:
			break;
		}

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, normalizedAlpha);
		switch (command.type) {
		case RenderType::Graph:
			DrawRotaGraphF(static_cast<int>(renderX), static_cast<int>(renderY), finalScale, drawAngle, command.handle, TRUE);
			break;
		case RenderType::Box:
			DrawBox(static_cast<int>(renderX), static_cast<int>(renderY), static_cast<int>(optX), static_cast<int>(optY), command.color, command.fill);
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
			DrawRectGraph(static_cast<int>(renderX), static_cast<int>(renderY),           // 描画先のX, Y (変換済み)
				static_cast<int>(command.srcX), static_cast<int>(command.srcY), // 元画像の切り出し開始X, Y
				static_cast<int>(command.srcWidth), static_cast<int>(command.srcHeight), // 元画像の切り出し幅, 高さ
				command.handle, TRUE); // グラフィックハンドル, TRUEで透過描画
			break;
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	m_commands.clear();
}

void RenderSystem::SetCameraView(MCameraComponent* m)
{
	m_MainCamera = m;
}

MCameraComponent* RenderSystem::GetCamera()
{
	return m_MainCamera;
}
