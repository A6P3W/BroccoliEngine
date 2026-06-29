#include "HttpManager.h"

#include <cpr/cpr.h>

#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

struct HttpResult {
  uint64_t RequestId;
  HttpResponse response;
  HttpCallback callback;
};

struct HttpManager::Impl {
  std::mutex queueMutex;
  std::vector<HttpResult> resultQueue;

  std::mutex activeMutex;
  std::unordered_map<uint64_t, const void*> activeRequests;
  uint64_t nextRequestId = 1;
};

HttpManager& HttpManager::GetInstance() {
  static HttpManager* instance = new HttpManager();
  return *instance;
}

HttpManager::HttpManager() { ImplPtr = std::make_shared<Impl>(); }

HttpManager::~HttpManager() {}

void HttpManager::Update() {
  std::vector<HttpResult> resultsToProcess;
  {
    std::lock_guard<std::mutex> lock(ImplPtr->queueMutex);
    if (!ImplPtr->resultQueue.empty()) {
      resultsToProcess = std::move(ImplPtr->resultQueue);
      ImplPtr->resultQueue.clear();
    }
  }

  for (auto& res : resultsToProcess) {
    bool bShouldExecute = false;
    {
      // 有効なリクエストかどうかを確認し、実行する場合はリストから消す
      std::lock_guard<std::mutex> lock(ImplPtr->activeMutex);
      auto it = ImplPtr->activeRequests.find(res.RequestId);
      if (it != ImplPtr->activeRequests.end()) {
        bShouldExecute = true;
        ImplPtr->activeRequests.erase(it);
      }
    }

    // キャンセルされていなければコールバックを実行
    if (bShouldExecute && res.callback) {
      res.callback(res.response);
    }
  }
}

void HttpManager::Get(const void* OwnerObject, const std::string& url, HttpCallback callback) {
  auto impl = ImplPtr;
  uint64_t reqId = 0;

  {
    std::lock_guard<std::mutex> lock(impl->activeMutex);
    reqId = impl->nextRequestId++;
    impl->activeRequests[reqId] = OwnerObject;
  }

  std::thread([impl, reqId, url, callback]() {
    cpr::Response r = cpr::Get(cpr::Url{url});
    HttpResponse response;
    response.StatusCode = r.status_code;
    response.Body = r.text;
    response.ErrorMessage = r.error.message;
    response.bSuccess = (r.status_code >= 200 && r.status_code < 300);

    std::lock_guard<std::mutex> lock(impl->queueMutex);
    impl->resultQueue.push_back({reqId, std::move(response), std::move(callback)});
  }).detach();
}

void HttpManager::PostJson(
    const void* OwnerObject,
    const std::string& url,
    const std::string& jsonBody,
    HttpCallback callback
) {
  auto impl = ImplPtr;
  uint64_t reqId = 0;

  {
    std::lock_guard<std::mutex> lock(impl->activeMutex);
    reqId = impl->nextRequestId++;
    impl->activeRequests[reqId] = OwnerObject;
  }

  std::thread([impl, reqId, url, jsonBody, callback]() {
    cpr::Response r = cpr::Post(
        cpr::Url{url}, cpr::Body{jsonBody}, cpr::Header{{"Content-Type", "application/json"}}
    );
    HttpResponse response;
    response.StatusCode = r.status_code;
    response.Body = r.text;
    response.ErrorMessage = r.error.message;
    response.bSuccess = (r.status_code >= 200 && r.status_code < 300);

    std::lock_guard<std::mutex> lock(impl->queueMutex);
    impl->resultQueue.push_back({reqId, std::move(response), std::move(callback)});
  }).detach();
}

void HttpManager::CancelAllRequestsForObject(const void* OwnerObject) {
  if (OwnerObject == nullptr) return;  // nullptr（グローバル通信）はキャンセルしない

  std::lock_guard<std::mutex> lock(ImplPtr->activeMutex);
  for (auto it = ImplPtr->activeRequests.begin(); it != ImplPtr->activeRequests.end();) {
    if (it->second == OwnerObject) {
      it = ImplPtr->activeRequests.erase(it);
    } else {
      ++it;
    }
  }
}
