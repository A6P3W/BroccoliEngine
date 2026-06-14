#include "SpriteComponent.h"
#include "Actor.h"
#include "UMath.h"
#include "ResourceManager.h"

MSpriteComponent::MSpriteComponent(int priority, RenderSpace space)
{
    m_common.priority = priority;
    m_common.space = space;
    m_common.alpha = 255;
    // デフォルトは空のグラフ
    m_data = GraphData{};
}

void MSpriteComponent::SubmitGraph(int handle, FScale scale, int alpha)
{
    m_data = GraphData{ {0,0}, FRotator(0), scale, handle};
    m_common.alpha = alpha;
}

void MSpriteComponent::SubmitBox(float width, float height, int color, bool fill, int alpha)
{
    m_data = BoxData{ {0,0}, {width, height}, FRotator(0), color, fill };
    m_common.alpha = alpha;
}

void MSpriteComponent::SubmitText(const std::string& text, int color, int handle, int alpha)
{
    int fontHandle = (handle != -1) ? handle : ResourceManager::GetInstance().GetFont(120, 5);
    m_data = TextData{ {0,0}, text, color, fontHandle };
    m_common.alpha = alpha;
}

void MSpriteComponent::SubmitLine(const FVector2D& relativeEnd, int color, int alpha)
{
    m_data = LineData{ {0,0}, relativeEnd, color };
    m_common.alpha = alpha;
}

void MSpriteComponent::SubmitRectGraph(float srcX, float srcY, float srcW, float srcH, int handle, int alpha)
{
    m_data = RectGraphData{ {0,0}, {srcX, srcY}, {srcW, srcH}, handle };
    m_common.alpha = alpha;
}

void MSpriteComponent::SubmitCircle(float radius, int color, bool fill, int alpha)
{
    m_data = CircleData{ {0,0}, radius, color, fill };
    m_common.alpha = alpha;
}

void MSpriteComponent::Draw()
{
    if (!bVisible) return;

    // 現在のワールドトランスフォームを取得
    FVector2D worldPos = GetWorldLocation();
    FRotator worldRot = GetWorldRotation();
    FScale worldScale = GetWorldScale();

    auto& rs = RenderSystem::GetInstance();

    std::visit([&](auto&& d) {
        using T = std::decay_t<decltype(d)>;

        if constexpr (std::is_same_v<T, GraphData>) {
            // アクタのスケールと回転を合成して送信
            rs.SubmitGraph(worldPos, d.Handle, d.Scale * worldScale, worldRot + d.Rotation,
                m_common.space, m_common.priority, m_common.alpha);
        }
        else if constexpr (std::is_same_v<T, BoxData>) {
            // Boxは左上座標(worldPos)とサイズ、回転を送信
            rs.SubmitBox(worldPos, d.WidthHeight, worldRot, d.Color, d.Fill,
                m_common.space, m_common.priority, m_common.alpha);
        }
        else if constexpr (std::is_same_v<T, CircleData>) {
            rs.SubmitCircle(worldPos, d.Radius * worldScale.Scale, d.Color, d.Fill,
                m_common.space, m_common.priority, m_common.alpha);
        }
        else if constexpr (std::is_same_v<T, TextData>) {
            rs.SubmitText(worldPos, d.Text, d.Handle, d.Color,
                m_common.space, m_common.priority, m_common.alpha);
        }
        else if constexpr (std::is_same_v<T, LineData>) {
            // LineData.EndLocation は相対座標として保持しているので、ワールド回転を考慮して終点を計算
            FVector2D rotatedEnd = d.EndLocation.RotateVector(worldRot) * worldScale;
            rs.SubmitLine(worldPos, worldPos + rotatedEnd, d.Color,
                m_common.space, m_common.priority, m_common.alpha);
        }
        else if constexpr (std::is_same_v<T, RectGraphData>) {
            rs.SubmitRectGraph(worldPos, d.SrcLocation, d.SrcSize, d.Handle,
                m_common.space, m_common.priority, m_common.alpha);
        }
        }, m_data);
}
