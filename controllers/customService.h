#pragma once
#include "controllerHeaders.h"

class customService : public drogon::HttpController<customService>
{
public:
  METHOD_LIST_BEGIN
  METHOD_ADD(customService::servicePageFunction, "/customServicePage");
  METHOD_ADD(customService::customServicePeerConnectionScript,"/customServicePeerConnection");
  METHOD_LIST_END
  void servicePageFunction(const HttpRequestPtr &req, CallBackType &&callback);
  void customServicePeerConnectionScript(const HttpRequestPtr &req, CallBackType &&callback);
};
