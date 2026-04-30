#pragma once
#include <string>
#include "UpdateableComponent.h"
class MUpdateableObject; // 前方宣言

struct Vector2
{
	float x = 0.0f;
	float y = 0.0f;
};

class MSceneComponent : public UpdateableComponent {
public:
	MSceneComponent();
	virtual ~MSceneComponent();
	virtual void OnUpdate(float DeltaTime);
	void Draw() override;

	// メッセージ受信用の仮想関数
	virtual void OnMessage(const std::string& message);

	void SetOwner(MUpdateableObject* owner);

	void SetPos(float nx, float ny);
	void SetPos(const Vector2& nPos);
	void AddLocalPos(float nx, float ny);
	void AddWorldPos(float nx, float ny);
	const Vector2& GetPos() const;

	void SetAngle(float nAngle);
	void AddAngle(float nAngleDeg);
	float GetAngle() const;

	void SetScale(float nScale);
	float GetScale() const;

protected:
	MUpdateableObject* m_owner = nullptr;
	Vector2 pos;
	float angle = 0.0f;
	float scale = 1.0f;
};