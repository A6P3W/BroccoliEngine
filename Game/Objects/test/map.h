#pragma once
#include <vector>
#include <string>
#include "GameObject.h"

class Map :public AGameObject {
public:
    static constexpr int TILE_SIZE = 32; // 1タイルのサイズ（ピクセル）

    Map() : Map(0.0f, 0.0f) {}
    Map(float x,float y);
    ~Map();

   
    bool Load(const std::string& csvPath, const std::string& chipPath);
    void OnDraw()override;

private:
    std::vector<std::vector<int>> mapData;
    int chipGraphHandle; // マップチップ画像ハンドル
    int mapWidth;        // マップの横方向タイル数
    int mapHeight;       // マップの縦方向タイル数
};