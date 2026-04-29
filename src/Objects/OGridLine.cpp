#include "Objects/OGridLine.h"
#include "Systems/RenderSystem.h"
#include "Objects/OCameraObject.h"
#include <DxLib.h>
#include<string>
#include <format>
#include "Systems/ResourceManager.h"
OGridLine::OGridLine(int LineWidth)
{
	m_LineWidth = LineWidth;
	m_LineColor = GetColor(255, 255, 255);
}

void OGridLine::OnDraw()
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
            (int)x,
            (int)(CamPos.y - WindowHeight / 2),
            (int)x,
            (int)(CamPos.y + WindowHeight / 2),
            m_LineColor,
            RenderSpace::World,
            -100,
            50
        );
        RenderSystem::GetInstance().SubmitText(
            std::format("{:.1f}",x*0.01),
            (int)x+2,
            (int)(CamPos.y + WindowHeight / 2-10),
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
            (int)(CamPos.x - WindowWidth / 2),
            (int)y,
            (int)(CamPos.x + WindowWidth / 2),
            (int)y,
            m_LineColor,
            RenderSpace::World,
            -100,
            50
        );
        RenderSystem::GetInstance().SubmitText(
            std::format("{:.1f}", y*0.01),
            (int)(CamPos.x - WindowWidth / 2+2),
            (int)y,
            m_LineColor,
            ResourceManager::GetInstance().GetFont(12, 5),
            RenderSpace::World,
            -100,
            200
        );
    }
}
