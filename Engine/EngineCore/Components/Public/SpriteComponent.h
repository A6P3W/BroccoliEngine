#pragma once
#include "SceneComponent.h"
#include "RenderSystem.h"

class MSpriteComponent : public MSceneComponent {
public:
    MSpriteComponent(int priority = 0, RenderSpace space = RenderSpace::World);

    void SubmitGraph(int handle, float scale = 1.0f, int alpha = 255);
    void SubmitBox(float width, float height, int color, bool fill, int alpha = 255);
    void SubmitText(const std::string& text, int color, int handle = -1, int alpha = 255);
    void SubmitLine(const FVector2D& relativeEnd, int color, int alpha = 255);
    void SubmitRectGraph(float srcX, float srcY, float srcW, float srcH, int handle, int alpha = 255);
    void SubmitCircle(float radius, int color, bool fill, int alpha = 255);

    void Draw() override;

private:
    // 描画設定（共通設定とデータ本体）
    RenderCommon m_common;
    RenderCommandData m_data;
};
