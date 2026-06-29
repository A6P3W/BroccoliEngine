#pragma once
#include <functional>
#include <memory>
#include <string>

struct HttpResponse {
  long StatusCode = 0;
  std::string Body;
  std::string ErrorMessage;
  bool bSuccess = false;
};

using HttpCallback = std::function<void(const HttpResponse&)>;

class HttpManager {
 public:
  static HttpManager& GetInstance();
  void Update();

  // 第一引数に OwnerObject を追加
  void Get(const void* OwnerObject, const std::string& url, HttpCallback callback);
  void PostJson(
      const void* OwnerObject,
      const std::string& url,
      const std::string& jsonBody,
      HttpCallback callback
  );

  // オブジェクトに紐づく全ての未完了コールバックをキャンセルする
  void CancelAllRequestsForObject(const void* OwnerObject);

 private:
  HttpManager();
  ~HttpManager();
  HttpManager(const HttpManager&) = delete;
  HttpManager& operator=(const HttpManager&) = delete;

  struct Impl;
  std::shared_ptr<Impl> ImplPtr;
};
