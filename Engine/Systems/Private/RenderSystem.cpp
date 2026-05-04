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

void RenderSystem::SubmitSprite(float x, float y, double Scale, double AngleDeg, int handle, RenderSpace space, int priority, int alpha)
{
	RenderCommand command;
	command.type = RenderType::Sprite;
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
	FVector2D CamPos = m_MainCamera->GetWorldLocation();
	float camAngle = m_MainCamera->GetWorldRotation().Rotation;
	int WindowWidth, WindowHeight;
	GetDrawScreenSize(&WindowWidth, &WindowHeight);
	const float centerX = WindowWidth * 0.5f;
	const float centerY = WindowHeight * 0.5f;
	const float rad = UMath::DegToRad(-camAngle);
	const float camAngleRad = UMath::DegToRad(camAngle);
	const float s = std::sin(rad);
	const float c = std::cos(rad);
	for (const auto& command : m_commands) {
		float x1 = command.x1;
		float y1 = command.y1;
		float x2 = command.x2;
		float y2 = command.y2;
		double drawAngle = command.AngleDeg;
		int normalizedAlpha = std::clamp(command.alpha, 0, 255);

		switch (command.space) {
		case RenderSpace::World:
			x1 -= CamPos.X;
			y1 -= CamPos.Y;
			x2 -= CamPos.X;
			y2 -= CamPos.Y;
			{
				float rx1 = x1 * c - y1 * s;
				float ry1 = x1 * s + y1 * c;
				float rx2 = x2 * c - y2 * s;
				float ry2 = x2 * s + y2 * c;
				x1 = rx1 + centerX;
				y1 = ry1 + centerY;
				x2 = rx2 + centerX;
				y2 = ry2 + centerY;
			}
			drawAngle -= camAngleRad;
			break;
		case RenderSpace::Screen:
			break;
		}

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, normalizedAlpha);
		switch (command.type) {
		case RenderType::Sprite:
         DrawRotaGraphF(static_cast<int>(x1), static_cast<int>(y1), command.Scale, drawAngle, command.handle, TRUE);
			break;
		case RenderType::Box:
           DrawBox(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2), command.color, command.fill);
			break;
		case RenderType::Text:
            DrawStringToHandle(static_cast<int>(x1), static_cast<int>(y1), command.text.c_str(), command.color, command.handle);
			break;
		case RenderType::Line:
            DrawLine(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2), command.color);
			break;
		case RenderType::RectGraph: 
			DrawRectGraph(static_cast<int>(x1), static_cast<int>(y1),           // 描画先のX, Y (変換済み)
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
