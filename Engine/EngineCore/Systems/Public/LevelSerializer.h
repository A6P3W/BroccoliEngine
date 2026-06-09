#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "UMath.h"

// 1アクタ分のデータ
struct FActorSaveData
{
	std::string ClassName;
	std::string InstanceName;
	FVector2D   Location;
	float       Rotation = 0.0f;
	float       Scale = 1.0f;

	// アクタ固有のプロパティ保存用
	std::unordered_map<std::string, std::string> CustomProperties;
};

class World;

class LevelSerializer
{
public:
	// 現在ObjectManagerにいる全アクタを保存
	// （ActorRegistryに登録されているクラスのみ対象）
	static bool Save(World* world, const std::string& filePath);

	// JSONからアクタをスポーン（現シーンはクリアしない）
	static bool Load(World* world, const std::string& filePath);

	// 低レベルAPI（EditorModeから直接データを渡す場合）
	static bool SaveData(const std::string& filePath,
		const std::vector<FActorSaveData>& actors);
	static bool LoadData(const std::string& filePath,
		std::vector<FActorSaveData>& outActors);
};
