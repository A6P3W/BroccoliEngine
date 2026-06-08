#pragma once
#include "Actor.h"
#include <string>

class MSpriteComponent;

class ASpriteActor : public AActor
{
public:
	DEFINE_ACTOR_CLASS(ASpriteActor);

	ASpriteActor();

	// 画像のファイルパスをセットして描画を更新する
	void SetImagePath(const std::string& path);
	const std::string& GetImagePath() const { return m_ImagePath; }

protected:
	void BeginPlay() override;

private:
	MSpriteComponent* m_SpriteComponent = nullptr;
	std::string m_ImagePath;
};
