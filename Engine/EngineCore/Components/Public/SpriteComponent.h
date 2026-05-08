#pragma once
#include "Components/Public/SceneComponent.h"
#include "RenderSystem.h"
class MSpriteComponent : public MSceneComponent {
public:
    MSpriteComponent(int priority = 0, RenderSpace space = RenderSpace::World);

    void SubmitGraph(double Scale, int handle, int alpha = 255);
    void SubmitBox(float width, float height, int color, int fill, int alpha = 255);
    void SubmitText(const std::string& text, int color, int handle, int alpha = 255);
    void SubmitLine(float x2, float y2, int color, int alpha = 255);
    void SubmitRectGraph(float srcX, float srcY, float srcW, float srcH, int handle, int alpha = 255);
    void SubmitCircle(float radius, int color, int fill, int alpha = 255);

    void Draw() override;

    void OnMessage(const std::string& message) override;

private:
    RenderCommand m_command;
};

