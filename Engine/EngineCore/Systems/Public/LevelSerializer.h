#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "UMath.h"

struct FLevelMetaData
{
	std::string GameModeClassName;
};

// 1アクタ分のデータ
struct FActorSaveData
{
	std::string ClassName;
	std::string InstanceName;
	FVector2D   Location;
	FRotator    Rotation = FRotator(0);
	FScale      Scale = FScale(1.0f);

	// アクタ固有のプロパティ保存用
	std::unordered_map<std::string, std::string> CustomProperties;
};

class World;

class LevelSerializer
{
public:
	// 現在ObjectManagerにいるActorRegistryに登録されている全アクタを保存
	static bool Save(World* world, const std::string& filePath, const std::string& gameModeClassName);

	// JSONからアクタをスポーン（現シーンはクリアしない）
	static bool Load(World* world, const std::string& filePath);
	static bool Load(World* world, const std::string& filePath, bool bLoadGameMode, FLevelMetaData* outMeta = nullptr);

	// 低レベルAPI（EditorModeから直接データを渡す場合）
	static bool SaveData(const std::string& filePath,
		const FLevelMetaData& meta,
		const std::vector<FActorSaveData>& actors);
	static bool LoadData(const std::string& filePath,
		std::vector<FActorSaveData>& outActors);
	static bool LoadData(const std::string& filePath,
		FLevelMetaData& outMeta,
		std::vector<FActorSaveData>& outActors);
};
