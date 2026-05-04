#include "Objects/GridLine.h"
#include "RenderSystem.h"
#include "CameraComponent.h"
#include <DxLib.h>
#include <string>
#include <format>
#include <cmath>
#include "ResourceManager.h"
#include "Utils/UMath.h"
AGridLine::AGridLine()
{
	
	m_LineColor = GetColor(255, 255, 255);
}

void AGridLine::OnDraw()
{
    auto Cam = RenderSystem::GetInstance().GetCamera();
    FVector2D CamPos = Cam->GetWorldLocation();
    float CamRotation = Cam->GetWorldRotation().Rotation;

    int WindowWidth, WindowHeight;
    GetDrawScreenSize(&WindowWidth, &WindowHeight);

	float GridWindowWidth = WindowWidth * 2.0f;
	float GridWindowHeight = WindowHeight * 2.0f;
    float margin = 5;
    float leftX = margin;
    float topY = margin;
    float rightX = WindowWidth - margin;
    float bottomY = WindowHeight - margin;
    if (rightX < leftX) rightX = leftX;
    if (bottomY < topY) bottomY = topY;

    float centerX = WindowWidth * 0.5f;
    float centerY = WindowHeight * 0.5f;
    float rad = UMath::DegToRad(-CamRotation);
    float s = std::sin(rad);
    float c = std::cos(rad);
    bool useBottomForX = std::abs(c) >= std::abs(s);
    bool useLeftForY = std::abs(c) >= std::abs(s);
    float minLabelSpacing = 40.0f;
    float lastXLabelPos = -10000.0f;
    float lastYLabelPos = -10000.0f;
    int xLabelColor = GetColor(255, 200, 0);
    int yLabelColor = GetColor(0, 200, 255);
    int locationText = ResourceManager::GetInstance().GetFont(12, 5);


    float startX = floor((CamPos.X - GridWindowWidth / 2) / m_LineWidth) * m_LineWidth;
    float startY = floor((CamPos.Y - GridWindowHeight / 2) / m_LineWidth) * m_LineWidth;

    int LineCountX = GridWindowWidth / m_LineWidth + 2;
    int LineCountY = GridWindowHeight / m_LineWidth + 2;

    auto SubmitAxisLabel = [&](const std::string& text, float x, float y, int color, int font) {
        RenderSystem::GetInstance().SubmitText(
            text,
            x,
            y,
            color,
            font,
            RenderSpace::Screen,
            -100,
            180
        );
    };

    RenderSystem::GetInstance().SubmitText(
        useBottomForX ? "X" : "Y",
        WindowWidth / 2,
        WindowHeight - 25.0f,
        useBottomForX ? xLabelColor : yLabelColor,
        ResourceManager::GetInstance().GetFont(24, 5),
        RenderSpace::Screen,
        -100,
        200
    );
    RenderSystem::GetInstance().SubmitText(
        useBottomForX ? "Y" : "X",
        5,
        WindowHeight / 2.0f,
        useBottomForX ? yLabelColor : xLabelColor,
        ResourceManager::GetInstance().GetFont(24, 5),
        RenderSpace::Screen,
        -100,
        200
    );
    // 垂直線
    for (int i = 0; i < LineCountX; i++) {
        float x = startX + i * m_LineWidth;
        RenderSystem::GetInstance().SubmitLine(
            x,
            CamPos.Y - GridWindowHeight / 2,
            x,
            CamPos.Y + GridWindowHeight / 2,
            m_LineColor,
            RenderSpace::World,
            -100,
            50
        );
        float dx = x - CamPos.X;
        if (useBottomForX && std::abs(c) > 0.0001f) {
            float ryTarget = bottomY - centerY;
            float dy = (ryTarget - dx * s) / c;
            float rx = dx * c - dy * s;
            float sx = rx + centerX;
            if (sx >= leftX && sx <= rightX && std::abs(sx - lastXLabelPos) >= minLabelSpacing) {
                SubmitAxisLabel(
                    std::format("{:.1f}", x * 0.01f),
                    sx + 2.0f,
                    bottomY - 10.0f,
                    xLabelColor,
                    locationText
                );
                lastXLabelPos = sx;
            }
        } else if (!useBottomForX && std::abs(s) > 0.0001f) {
            float rxTarget = leftX - centerX;
            float dy = (dx * c - rxTarget) / s;
            float ry = dx * s + dy * c;
            float sy = ry + centerY;
            if (sy >= topY && sy <= bottomY && std::abs(sy - lastXLabelPos) >= minLabelSpacing) {
                SubmitAxisLabel(
                    std::format("{:.1f}", x * 0.01f),
                    leftX + 2.0f,
                    sy,
                    xLabelColor,
                    locationText
                );
                lastXLabelPos = sy;
            }
        }
    }

    // 水平線
    for (int i = 0; i < LineCountY; i++) {
        float y = startY + i * m_LineWidth;
        RenderSystem::GetInstance().SubmitLine(
            CamPos.X - GridWindowWidth / 2,
            y,
            CamPos.X + GridWindowWidth / 2,
            y,
            m_LineColor,
            RenderSpace::World,
            -100,
            50
        );
        float dy = y - CamPos.Y;
        if (useLeftForY && std::abs(c) > 0.0001f) {
            float rxTarget = leftX - centerX;
            float dx = (rxTarget + dy * s) / c;
            float ry = dx * s + dy * c;
            float sy = ry + centerY;
            if (sy >= topY && sy <= bottomY && std::abs(sy - lastYLabelPos) >= minLabelSpacing) {
                SubmitAxisLabel(
                    std::format("{:.1f}", y * 0.01f),
                    leftX + 2.0f,
                    sy,
                    yLabelColor,
                    locationText
                );
                lastYLabelPos = sy;
            }
        } else if (!useLeftForY && std::abs(s) > 0.0001f) {
            float ryTarget = bottomY - centerY;
            float dx = (ryTarget - dy * c) / s;
            float rx = dx * c - dy * s;
            float sx = rx + centerX;
            if (sx >= leftX && sx <= rightX && std::abs(sx - lastYLabelPos) >= minLabelSpacing) {
                SubmitAxisLabel(
                    std::format("{:.1f}", y * 0.01f),
                    sx + 2.0f,
                    bottomY - 10.0f,
                    yLabelColor,
                    locationText
                );
                lastYLabelPos = sx;
            }
        }
    }
}

void AGridLine::SetLineWidth(float LineWidth)
{
    m_LineWidth = LineWidth;
}
