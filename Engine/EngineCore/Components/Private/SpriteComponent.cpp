#include "SpriteComponent.h"

#include "Actor.h"
#include "ResourceManager.h"
#include "UMath.h"

class SpriteRenderState {
 public:
  RenderCommon CommonData;
  RenderCommandData FeatureData;
};

MSpriteComponent::MSpriteComponent() : RenderState(new SpriteRenderState()) {
  RenderState->CommonData.priority = 0;
  RenderState->CommonData.space = RenderSpace::World;
  RenderState->CommonData.alpha = 255;
  // デフォルトは空のグラフ
  RenderState->FeatureData = GraphData{};
}

MSpriteComponent::~MSpriteComponent() { delete RenderState; }

void MSpriteComponent::SetRenderSettings(int Priority, RenderSpace Space) {
  RenderState->CommonData.priority = Priority;
  RenderState->CommonData.space = Space;
}

void MSpriteComponent::SubmitGraph(int handle, FScale scale, int alpha) {
  RenderState->FeatureData = GraphData{{0, 0}, FRotator(0), scale, handle};
  RenderState->CommonData.alpha = alpha;
}

void MSpriteComponent::SubmitBox(float width, float height, const FColor& color, bool fill) {
  RenderState->FeatureData = BoxData{{0, 0}, {width, height}, FRotator(0), color, fill};
  RenderState->CommonData.alpha = static_cast<int>(color.A);
}

void MSpriteComponent::SubmitText(const std::string& text, const FColor& color, int handle) {
  int fontHandle = (handle != -1) ? handle : ResourceManager::GetInstance().GetFont(120, 5);
  RenderState->FeatureData = TextData{{0, 0}, text, color, fontHandle};
  RenderState->CommonData.alpha = static_cast<int>(color.A);
}

void MSpriteComponent::SubmitLine(const FVector2D& relativeEnd, const FColor& color) {
  RenderState->FeatureData = LineData{{0, 0}, relativeEnd, color};
  RenderState->CommonData.alpha = static_cast<int>(color.A);
}

void MSpriteComponent::SubmitRectGraph(
    float srcX, float srcY, float srcW, float srcH, int handle, int alpha
) {
  RenderState->FeatureData = RectGraphData{{0, 0}, {srcX, srcY}, {srcW, srcH}, handle};
  RenderState->CommonData.alpha = alpha;
}

void MSpriteComponent::SubmitCircle(float radius, const FColor& color, bool fill) {
  RenderState->FeatureData = CircleData{{0, 0}, radius, color, fill};
  RenderState->CommonData.alpha = static_cast<int>(color.A);
}

void MSpriteComponent::Draw() {
  if (!IsVisible()) return;

  // 現在のワールドトランスフォームを取得
  FVector2D worldPos = GetWorldLocation();
  FRotator worldRot = GetWorldRotation();
  FScale worldScale = GetWorldScale();

  auto& rs = RenderSystem::GetInstance();

  std::visit(
      [&](auto&& d) {
        using T = std::decay_t<decltype(d)>;

        if constexpr (std::is_same_v<T, GraphData>) {
          // アクタのスケールと回転を合成して送信
          rs.SubmitGraph(
              worldPos,
              d.Handle,
              d.Scale * worldScale,
              worldRot + d.Rotation,
              RenderState->CommonData.space,
              RenderState->CommonData.priority,
              RenderState->CommonData.alpha
          );
        } else if constexpr (std::is_same_v<T, BoxData>) {
          const FVector2D ScaledSize = d.WidthHeight * worldScale;
          FVector2D TopLeftOffset = ScaledSize * -0.5f;
          if (RenderState->CommonData.space == RenderSpace::World) {
            TopLeftOffset = TopLeftOffset.RotateVector(worldRot);
          }

          rs.SubmitBox(
              worldPos + TopLeftOffset,
              ScaledSize,
              worldRot,
              d.Color,
              d.Fill,
              RenderState->CommonData.space,
              RenderState->CommonData.priority
          );
        } else if constexpr (std::is_same_v<T, CircleData>) {
          rs.SubmitCircle(
              worldPos,
              d.Radius * worldScale.Scale,
              d.Color,
              d.Fill,
              RenderState->CommonData.space,
              RenderState->CommonData.priority
          );
        } else if constexpr (std::is_same_v<T, TextData>) {
          rs.SubmitText(
              worldPos,
              d.Text,
              d.Handle,
              d.Color,
              RenderState->CommonData.space,
              RenderState->CommonData.priority
          );
        } else if constexpr (std::is_same_v<T, LineData>) {
          // LineData.EndLocation は相対座標として保持しているので、ワールド回転を考慮して終点を計算
          FVector2D rotatedEnd = d.EndLocation.RotateVector(worldRot) * worldScale;
          rs.SubmitLine(
              worldPos,
              worldPos + rotatedEnd,
              d.Color,
              RenderState->CommonData.space,
              RenderState->CommonData.priority
          );
        } else if constexpr (std::is_same_v<T, RectGraphData>) {
          rs.SubmitRectGraph(
              worldPos,
              d.SrcLocation,
              d.SrcSize,
              d.Handle,
              RenderState->CommonData.space,
              RenderState->CommonData.priority,
              RenderState->CommonData.alpha
          );
        }
      },
      RenderState->FeatureData
  );
}
