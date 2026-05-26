#pragma once
#include <string>
#include <vector>
#include "Utils/UMath.h"

// 1アクタ分のデータ
struct FActorSaveData
{
	std::string ClassName;
	FVector2D   Location;
	float       Rotation = 0.0f;
	float       Scale = 1.0f;
};

class LevelSerializer
{
public:
	// 現在ObjectManagerにいる全アクタを保存
	// （ActorRegistryに登録されているクラスのみ対象）
	static bool Save(const std::string& filePath);

	// JSONからアクタをスポーン（現シーンはクリアしない）
	static bool Load(const std::string& filePath);

	// 低レベルAPI（EditorModeから直接データを渡す場合）
	static bool SaveData(const std::string& filePath,
		const std::vector<FActorSaveData>& actors);
	static bool LoadData(const std::string& filePath,
		std::vector<FActorSaveData>& outActors);
};
