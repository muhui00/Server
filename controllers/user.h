#pragma once

#include "controllerHeaders.h"

class user : public drogon::HttpController<user>
{
public:
  static std::unordered_map<int, vector<int>> estateId2DeviceInfo;
  static DBThreadPoolPtr pool;
  METHOD_LIST_BEGIN
  METHOD_ADD(user::userLoginPageFunction, "/userLoginPage", Get);
  METHOD_ADD(user::userLoginFunction, "/userLogin", Post);
  METHOD_ADD(user::userMainPageFunction, "/userMainPage", Get);
  METHOD_ADD(user::selectEstatePageFunction, "/selectEstatePage", Get);
  METHOD_ADD(user::selectEstateFunction, "/selectEstate?estate_id={1}", Get);
  METHOD_ADD(user::peerConnectionScript,"/peerconnection",Get);
  METHOD_LIST_END
  void peerConnectionScript(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);
  void selectEstateFunction(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback,
                            int estate_id);
  void selectEstatePageFunction(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback);
  const string userMainPagePath = "../front/userMainPage.html";
  void userMainPageFunction(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);
  const string userLoginPagePath = "../front/userLoginPage.html";
  void userLoginPageFunction(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback);
  void userLoginFunction(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback);
};
