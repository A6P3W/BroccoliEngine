#include "Objects/GridLine.h"
#include "Systems/RenderSystem.h"
#include "Objects/CameraObject.h"
#include <DxLib.h>
#include<string>
#include <format>
#include "Systems/ResourceManager.h"
AGridLine::AGridLine(int LineWidth)
{
	m_LineWidth = LineWidth;
	m_LineColor = GetColor(255, 255, 255);
}

void AGridLine::OnDraw()
{
    Vector2 CamPos = RenderSystem::GetInstance().GetCamera()->GetTransform()->GetPos();

    int WindowWidth, WindowHeight;
    GetDrawScreenSize(&WindowWidth, &WindowHeight);

    float startX = floor((CamPos.x - WindowWidth / 2) / m_LineWidth) * m_LineWidth;
    float startY = floor((CamPos.y - WindowHeight / 2) / m_LineWidth) * m_LineWidth;

    int LineCountX = WindowWidth / m_LineWidth + 2;
    int LineCountY = WindowHeight / m_LineWidth + 2;

    // 垂直線
    for (int i = 0; i < LineCountX; i++) {
        float x = startX + i * m_LineWidth;
        RenderSystem::GetInstance().SubmitLine(
            x,
            CamPos.y - WindowHeight / 2,
            x,
            CamPos.y + WindowHeight / 2,
            m_LineColor,
            RenderSpace::World,
            -100,
            50
        );
        RenderSystem::GetInstance().SubmitText(
            std::format("{:.1f}",x*0.01),
            x+2,
            CamPos.y + WindowHeight / 2-10,
            m_LineColor,
            ResourceManager::GetInstance().GetFont(12,5),
            RenderSpace::World,
            -100,
            200
        );
    }

    // 水平線
    for (int i = 0; i < LineCountY; i++) {
        float y = startY + i * m_LineWidth;
        RenderSystem::GetInstance().SubmitLine(
            CamPos.x - WindowWidth / 2,
            y,
            CamPos.x + WindowWidth / 2,
            y,
            m_LineColor,
            RenderSpace::World,
            -100,
            50
        );
        RenderSystem::GetInstance().SubmitText(
            std::format("{:.1f}", y*0.01),
            CamPos.x - WindowWidth / 2+2,
            y,
            m_LineColor,
            ResourceManager::GetInstance().GetFont(12, 5),
            RenderSpace::World,
            -100,
            200
        );
    }
}
