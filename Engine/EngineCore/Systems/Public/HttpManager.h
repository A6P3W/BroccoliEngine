#pragma once
#include <string>
#include <functional>

// 通信結果を格納する構造体
struct HttpResponse {
    long StatusCode = 0;           // 200, 404, 500 など
    std::string Body;              // サーバーからのレスポンス(JSON文字列など)
    std::string ErrorMessage;      // 通信エラー時のメッセージ
    bool bSuccess = false;         // StatusCode が 200番台なら true
};

// リクエスト完了時に呼び出されるコールバックの型
using HttpCallback = std::function<void(const HttpResponse&)>;

class HttpManager {
public:
    static HttpManager& GetInstance();

    // 毎フレーム呼び出して、完了した通信のコールバックを実行する
    void Update();

    // HTTP GETリクエスト
    void Get(const std::string& url, HttpCallback callback);

    // HTTP POSTリクエスト (JSON送信)
    void PostJson(const std::string& url, const std::string& jsonBody, HttpCallback callback);

private:
    HttpManager();
    ~HttpManager();
    HttpManager(const HttpManager&) = delete;
    HttpManager& operator=(const HttpManager&) = delete;

    // 実装を隠蔽するためのポインタ
    struct Impl;
    Impl* m_Impl = nullptr;
};
