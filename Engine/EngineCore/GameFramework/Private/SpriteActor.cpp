#include "SpriteActor.h"
#include "SpriteComponent.h"
#include "ResourceManager.h"

// ClassBrowserに自動表示されるように登録
REGISTER_ACTOR(ASpriteActor);

ASpriteActor::ASpriteActor()
{
	auto spriteComp = std::make_unique<MSpriteComponent>();
	m_SpriteComponent = spriteComp.get();

	// スプライトをルートコンポーネントに設定
	SetRootComponent(m_SpriteComponent);
	AddComponent(std::move(spriteComp));
}

void ASpriteActor::SetImagePath(const std::string& path)
{
	m_ImagePath = path;
	if (m_SpriteComponent)
	{
		int handle = ResourceManager::GetInstance().LoadResourceGraph(path);
		if (handle != -1)
		{
			// 等倍、不透明で画像をセット
			m_SpriteComponent->SubmitGraph(handle, 1.0f, 255);
		}
	}
}

void ASpriteActor::BeginPlay()
{
	AActor::BeginPlay();

	if (m_ImagePath.empty())
	{
		// プロジェクト内にある適当なデフォルト画像（チェッカー柄など）を指定
		SetImagePath("Engine/EngineSide/Files/texture_Checker_64px.png");
	}
	else
	{
		SetImagePath(m_ImagePath);
	}
}
