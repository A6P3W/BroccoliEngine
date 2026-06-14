#include "HttpManager.h"
#include <cpr/cpr.h>
#include <thread>
#include <mutex>
#include <vector>

struct HttpResult {
    HttpResponse response;
    HttpCallback callback;
};

struct HttpManager::Impl {
    std::mutex queueMutex;
    std::vector<HttpResult> resultQueue;
};

HttpManager& HttpManager::GetInstance() {
    static HttpManager instance;
    return instance;
}

HttpManager::HttpManager() {
    m_Impl = new Impl();
}

HttpManager::~HttpManager() {
    delete m_Impl;
}

void HttpManager::Update() {
    std::vector<HttpResult> resultsToProcess;

    // ロックを掛けて、現在のキューをすべて取り出す
    {
        std::lock_guard<std::mutex> lock(m_Impl->queueMutex);
        if (!m_Impl->resultQueue.empty()) {
            resultsToProcess = std::move(m_Impl->resultQueue);
            m_Impl->resultQueue.clear();
        }
    }

    // メインスレッドでコールバックを実行
    for (auto& res : resultsToProcess) {
        if (res.callback) {
            res.callback(res.response);
        }
    }
}

void HttpManager::Get(const std::string& url, HttpCallback callback) {
    // スレッドを分離して裏で通信を実行
    std::thread([this, url, callback]() {
        cpr::Response r = cpr::Get(cpr::Url{ url });

        HttpResponse response;
        response.StatusCode = r.status_code;
        response.Body = r.text;
        response.ErrorMessage = r.error.message;
        response.bSuccess = (r.status_code >= 200 && r.status_code < 300);

        // 結果をスレッドセーフにキューに追加
        std::lock_guard<std::mutex> lock(m_Impl->queueMutex);
        m_Impl->resultQueue.push_back({ std::move(response), std::move(callback) });

        }).detach();
}

void HttpManager::PostJson(const std::string& url, const std::string& jsonBody, HttpCallback callback) {
    std::thread([this, url, jsonBody, callback]() {
        cpr::Response r = cpr::Post(
            cpr::Url{ url },
            cpr::Body{ jsonBody },
            cpr::Header{ {"Content-Type", "application/json"} }
        );

        HttpResponse response;
        response.StatusCode = r.status_code;
        response.Body = r.text;
        response.ErrorMessage = r.error.message;
        response.bSuccess = (r.status_code >= 200 && r.status_code < 300);

        std::lock_guard<std::mutex> lock(m_Impl->queueMutex);
        m_Impl->resultQueue.push_back({ std::move(response), std::move(callback) });

        }).detach();
}
