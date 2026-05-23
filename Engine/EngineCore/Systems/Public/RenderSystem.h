#pragma once
#include <vector>
#include <string>
#include <map>
#include "Utils/UMath.h"
enum class RenderType {
    Graph,
    Box,
    Text,
	Line,
    RectGraph,
    Circle
};
enum class RenderSpace {
    World, 
    Screen  
};

struct RenderCommand {
    int priority = 0;
    RenderType type = RenderType::Graph;

    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    int handle = -1;
    int color = 0;
    int fill = 1;
    int alpha = 255;
    double Scale = 1.0;
    double AngleDeg = 0.0;
    float srcX = 0.0f;
    float srcY = 0.0f;
    float srcWidth = 0.0f;
    float srcHeight = 0.0f;
    std::string text;
    RenderSpace space = RenderSpace::World;
};

class MCameraComponent;

class RenderSystem
{
public:
    RenderSystem();
    static RenderSystem& GetInstance();

    void SubmitGraph(float x, float y, double Scale, double AngleDeg, int handle, RenderSpace space , int priority, int alpha = 255);
    void SubmitBox(float x1, float y1, float x2, float y2, int color, int fill, RenderSpace space , int priority, int alpha = 255);
    void SubmitText(const std::string& text, float x, float y, int color, int handle, RenderSpace space ,int priority, int alpha = 255);
    void SubmitLine(float x1, float y1, float x2, float y2, int color, RenderSpace space, int priority, int alpha = 255);
    void SubmitRectGraph(float destX, float destY, float srcX, float srcY, float srcW, float srcH,int handle, RenderSpace space, int priority, int alpha = 255);
    void SubmitCircle(float x, float y, float radius, int color, int fill, RenderSpace space, int priority, int alpha = 255);
    FVector2D WorldToScreen(const FVector2D& worldPosition) const;
    FVector2D ScreenToWorld(const FVector2D& screenPosition) const;

    void Draw();

    void SetCameraView(MCameraComponent* m);
    MCameraComponent* GetCamera();

private:
    std::map<int, std::vector<RenderCommand>> m_priorityCommands;
    MCameraComponent* m_MainCamera = nullptr;
};
