#include "SampleScene01.h"
#include "ObjectManager.h"
#include "UMath.h"
#include "HttpManager.h"
#include "Log.h"
#include "nlohmann/json.hpp"
#include <SpriteComponent.h>
#include <PlayerController.h>
#include "SamplePawn01.h"
ASampleScene01::ASampleScene01() {

}

void ASampleScene01::BeginPlay()
{
	SpawnPlayer<ASamplePawn01, APlayerController>({ 0,0 }, 0);
    HttpManager::GetInstance().Get("https://jsonplaceholder.typicode.com/todos/1", [](const HttpResponse& response) {
        if (response.bSuccess) {
            M_LOG("通信成功！: {}", response.Body);

            // JSONとしてパースする例
            try {
                auto j = nlohmann::json::parse(response.Body);
                std::string title = j["title"];
                M_LOG("Title: {}", title);
            }
            catch (...) {
                M_LOG("JSONのパースに失敗");
            }
        }
        else {
            M_LOG("通信失敗... Status: {}, Error: {}", response.StatusCode, response.ErrorMessage);
        }
        });
}

void ASampleScene01::OnUpdate(float DeltaTime)
{
}
