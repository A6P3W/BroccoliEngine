#pragma once
#include <vector>
#include <string>

enum class RenderType {
    Sprite,
    Box,
    Text,
	Line
};
enum class RenderSpace {
    World, 
    Screen  
};

struct RenderCommand {
    int priority = 0;
    RenderType type = RenderType::Sprite;

    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    int handle = -1;
    int color = 0;
    int fill = 1;
    int alpha = 255;
    double scale = 1.0;
    double angle = 0.0;
    std::string text;
    RenderSpace space = RenderSpace::World;
};

class OCameraObject;

class RenderSystem
{
public:
    RenderSystem();
    static RenderSystem& GetInstance();

    void SubmitSprite(float x, float y, double scale, double angle, int handle, RenderSpace space , int priority, int alpha = 255);
    void SubmitBox(float x1, float y1, float x2, float y2, int color, int fill, RenderSpace space , int priority, int alpha = 255);
    void SubmitText(const std::string& text, float x, float y, int color, int handle, RenderSpace space ,int priority, int alpha = 255);
    void SubmitLine(float x1, float y1, float x2, float y2, int color, RenderSpace space, int priority, int alpha = 255);

    void Draw();

    void SetCameraView(OCameraObject*m);
    OCameraObject* GetCamera();

private:
    std::vector<RenderCommand> m_commands;
    OCameraObject* m_MainCamera = nullptr;
};
