#pragma once

#include "controllerHeaders.h"
class estate : public drogon::HttpController<estate>
{

public:
  static std::shared_ptr<sql::Connection> conn;
  METHOD_LIST_BEGIN
  METHOD_ADD(estate::estateListPageFunction, "/estateListPage", Get);
  METHOD_ADD(estate::searchEstateFunction, "/searchEstate?offset={1}&num={2}", Get);
  METHOD_ADD(estate::modifyEstateInfo, "/modifyEstateInfo", Post);
  METHOD_ADD(estate::createNewEstateInfo, "/createNewEstateInfo", Post);
  METHOD_ADD(estate::createNewEstateInfoPage, "/createNewEstateInfoPage", Get);
  METHOD_LIST_END
  void createNewEstateInfoPage(const HttpRequestPtr &req, CallBackType &&callback);
  void createNewEstateInfo(const HttpRequestPtr &req, CallBackType &&callback);
  void modifyEstateInfo(const HttpRequestPtr &req, CallBackType &&callback);
  void searchEstateFunction(const HttpRequestPtr &req, CallBackType &&callback, int offset, int num);
  void estateListPageFunction(const HttpRequestPtr &req, CallBackType &&callback);
};
