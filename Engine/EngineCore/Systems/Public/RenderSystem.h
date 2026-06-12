#pragma once
#include <vector>
#include <string>
#include <map>
#include <variant>
#include "UMath.h"
enum class RenderType {
	Graph,
	Box,
	Text,
	Line,
	RectGraph,
	Circle
};
enum class RenderSpace { World, Screen };

struct GraphData { FVector2D Location; FRotator Rotation; FScale Scale; int Handle=0; };
struct BoxData { FVector2D Location; FVector2D WidthHeight; FRotator Rotation; int Color; bool Fill; };
struct TextData { FVector2D Location; std::string Text; int Color; int Handle=0; };
struct LineData { FVector2D StartLocation; FVector2D EndLocation; int Color; };
struct CircleData { FVector2D Location; float Radius; int Color; bool Fill; };
struct RectGraphData { FVector2D DestLocation; FVector2D SrcLocation; FVector2D SrcSize; int Handle=0; };

// Variant でまとめる
using RenderCommandData = std::variant<GraphData, BoxData, TextData, LineData, CircleData, RectGraphData>;

struct RenderCommon {
	int priority = 0;
	int alpha = 255;
	RenderSpace space = RenderSpace::World;
};

struct RenderCommand {
	RenderCommon common;
	RenderCommandData data;
};

class MCameraComponent;

class RenderSystem {
public:
	RenderSystem();
	static RenderSystem& GetInstance();

	void SubmitLine(FVector2D Start, FVector2D End, int Color, RenderSpace Space, int Priority, int Alpha = 255);
	void SubmitBox(FVector2D Location, FVector2D WidthHeight, FRotator Rotation, int Color, bool Fill, RenderSpace Space, int Priority, int Alpha = 255);
	void SubmitCircle(FVector2D Location, float Radius, int Color, bool Fill, RenderSpace Space, int Priority, int Alpha = 255);
	void SubmitText(FVector2D Location, const std::string& Text, int Handle, int Color, RenderSpace Space, int Priority, int Alpha = 255);
	void SubmitGraph(FVector2D Location, int Handle, FScale Scale, FRotator Rotation, RenderSpace Space, int Priority, int Alpha = 255);
	void SubmitRectGraph(FVector2D Dest, FVector2D SrcLoc, FVector2D SrcSize, int Handle, RenderSpace Space, int Priority, int Alpha = 255);

	FVector2D WorldToScreen(const FVector2D& worldPosition) const;
	FVector2D ScreenToWorld(const FVector2D& screenPosition) const;
	void Draw();
	void SetCameraView(MCameraComponent* m);
	MCameraComponent* GetCamera();

private:
	std::map<int, std::vector<RenderCommand>> m_priorityCommands;
	MCameraComponent* m_MainCamera = nullptr;
};
