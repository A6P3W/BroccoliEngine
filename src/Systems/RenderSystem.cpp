#include "Systems/RenderSystem.h"
#include "DxLib.h"
#include <algorithm>
#include "Objects/CameraObject.h"
RenderSystem::RenderSystem()
{
}
RenderSystem& RenderSystem::GetInstance()
{
    static RenderSystem instance;
    return instance;
}

void RenderSystem::SubmitSprite(float x, float y, double scale, double angle, int handle,RenderSpace space ,int priority, int alpha)
{
    RenderCommand command;
    command.type = RenderType::Sprite;
    command.x1 = x;
    command.y1 = y;
    command.scale = scale;
    command.angle = angle;
    command.handle = handle;
    command.space = space;
    command.priority = priority;
    command.alpha = alpha;
    m_commands.push_back(std::move(command));
}

void RenderSystem::SubmitBox(float x1, float y1, float x2, float y2, int color, int fill, RenderSpace space , int priority, int alpha)
{
    RenderCommand command;
    command.type = RenderType::Box;
    command.x1 = x1;
    command.y1 = y1;
    command.x2 = x2;
    command.y2 = y2;
    command.color = color;
    command.fill = fill;
    command.priority = priority;
    command.alpha = alpha;
    m_commands.push_back(std::move(command));
}

void RenderSystem::SubmitText(const std::string& text, float x, float y, int color, int handle, RenderSpace space , int priority, int alpha)
{
    RenderCommand command;
    command.type = RenderType::Text;
    command.text = text;
    command.x1 = x;
    command.y1 = y;
    command.handle = handle;
    command.color = color;
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

void RenderSystem::Draw()
{
    std::stable_sort(m_commands.begin(), m_commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
        return a.priority < b.priority;
    });
    Vector2 CamPos=(m_MainCamera->GetTransform()->GetPos());
	int WindowWidth, WindowHeight;
	GetDrawScreenSize(&WindowWidth, &WindowHeight);
    const float offsetX = (WindowWidth * 0.5f) - CamPos.x;
    const float offsetY = (WindowHeight * 0.5f) - CamPos.y;
    for (const auto& command : m_commands) {
        float x1 = command.x1;
        float y1 = command.y1;
        float x2 = command.x2;
        float y2 = command.y2;
        int normalizedAlpha = std::clamp(command.alpha, 0, 255);

        switch (command.space) {
        case RenderSpace::World:
            x1 += offsetX;
            y1 += offsetY;
            x2 += offsetX;
            y2 += offsetY;
            break;
        case RenderSpace::Screen:
            break;
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, normalizedAlpha);
        switch (command.type) {
        case RenderType::Sprite:
            DrawRotaGraphF(x1, y1, command.scale, command.angle, command.handle, TRUE);
            break;
        case RenderType::Box:
            DrawBox(x1, y1, x2, y2, command.color, command.fill);
            break;
        case RenderType::Text:
            DrawStringToHandle(x1, y1, command.text.c_str(), command.color,command.handle);
            break;
        case RenderType::Line:
            DrawLine(x1, y1, x2, y2, command.color);
			break;
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    m_commands.clear();
}

void RenderSystem::SetCameraView(ACameraObject* m)
{
    m_MainCamera = m;
}

ACameraObject* RenderSystem::GetCamera()
{
	return m_MainCamera;
}
